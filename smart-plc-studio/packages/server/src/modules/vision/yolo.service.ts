import * as path from 'node:path'
import * as fs from 'node:fs'
import { fileURLToPath } from 'node:url'
import { PNG } from 'pngjs'
import jpeg from 'jpeg-js'

// 兼容两种运行形态：esbuild CJS bundle 下用 __dirname；tsx/ESM 下用 import.meta.url
function moduleDir(): string {
  if (typeof __dirname !== 'undefined') return __dirname
  return path.dirname(fileURLToPath(import.meta.url))
}
const MODULE_DIR = moduleDir()

// ===== YOLO26 目标检测（onnxruntime-node） =====
// 默认 ONNX 导出为端到端头，输出 (1, 300, 6)，每行 [x1, y1, x2, y2, conf, class_id]，
// 坐标在 letterbox 后的 imgsz 图上，无需 NMS。

export interface YoloDetection {
  classId: number
  className: string
  score: number
  x: number
  y: number
  width: number
  height: number
}

export interface YoloOptions {
  model: string
  conf?: number
  iou?: number
  imgsz?: number
}

// COCO 80 类（YOLO26 预训练权重默认类别）
const COCO_CLASSES = [
  'person', 'bicycle', 'car', 'motorcycle', 'airplane', 'bus', 'train', 'truck', 'boat',
  'traffic light', 'fire hydrant', 'stop sign', 'parking meter', 'bench', 'bird', 'cat', 'dog',
  'horse', 'sheep', 'cow', 'elephant', 'bear', 'zebra', 'giraffe', 'backpack', 'umbrella',
  'handbag', 'tie', 'suitcase', 'frisbee', 'skis', 'snowboard', 'sports ball', 'kite',
  'baseball bat', 'baseball glove', 'skateboard', 'surfboard', 'tennis racket', 'bottle',
  'wine glass', 'cup', 'fork', 'knife', 'spoon', 'bowl', 'banana', 'apple', 'sandwich',
  'orange', 'broccoli', 'carrot', 'hot dog', 'pizza', 'donut', 'cake', 'chair', 'couch',
  'potted plant', 'bed', 'dining table', 'toilet', 'tv', 'laptop', 'mouse', 'remote',
  'keyboard', 'cell phone', 'microwave', 'oven', 'toaster', 'sink', 'refrigerator', 'book',
  'clock', 'vase', 'scissors', 'teddy bear', 'hair drier', 'toothbrush',
]

export function cocoClassName(id: number): string {
  return COCO_CLASSES[id] ?? `class_${id}`
}

// ===== 模型目录解析 =====
// 在 dev(tsx from packages/server)、构建(tsc dist) 与 Electron(dist-server bundle) 三种
// 环境下定位 models 目录，支持环境变量覆盖。

// 向上查找 workspace 根（含 workspaces 字段的 package.json），避免依赖启动 cwd
function findWorkspaceRoot(start: string): string {
  let dir = start
  for (let i = 0; i < 6; i++) {
    try {
      const pkgFile = path.join(dir, 'package.json')
      if (fs.existsSync(pkgFile)) {
        const pkg = JSON.parse(fs.readFileSync(pkgFile, 'utf8'))
        if (pkg.workspaces) return dir
      }
    } catch {
      /* 继续向上 */
    }
    dir = path.dirname(dir)
  }
  return ''
}

function modelsDirCandidates(): string[] {
  const out: string[] = []
  if (process.env.PLC_MODELS_DIR) out.push(process.env.PLC_MODELS_DIR)
  const root = findWorkspaceRoot(MODULE_DIR)
  if (root) out.push(path.join(root, 'packages/server/models'))
  out.push(path.join(process.cwd(), 'packages/server/models'))
  out.push(path.join(process.cwd(), 'models'))
  return out
}

export function getModelsDir(): string {
  for (const dir of modelsDirCandidates()) {
    try {
      if (fs.existsSync(dir) && fs.statSync(dir).isDirectory()) return dir
    } catch {
      /* 继续尝试 */
    }
  }
  const fallback = modelsDirCandidates()[0]
  fs.mkdirSync(fallback, { recursive: true })
  return fallback
}

export function listYoloModels(): string[] {
  const dir = getModelsDir()
  try {
    return fs
      .readdirSync(dir)
      .filter((f) => f.toLowerCase().endsWith('.onnx'))
      .sort()
  } catch {
    return []
  }
}

function resolveModelPath(model: string): string {
  const name = path.basename(model)
  const full = path.join(getModelsDir(), name)
  if (!fs.existsSync(full)) {
    throw new Error(`未找到模型 ${name}，请将 .onnx 模型放入 ${getModelsDir()}（可用 ${listYoloModels().join(', ') || '暂无'}）`)
  }
  return full
}

// ===== onnxruntime 懒加载 =====
let ortMod: any | null = null

async function loadOrt(): Promise<any> {
  if (ortMod) return ortMod
  try {
    ortMod = await import('onnxruntime-node')
  } catch (e: any) {
    throw new Error('onnxruntime-node 未安装或加载失败: ' + (e?.message ?? String(e)))
  }
  return ortMod
}

const sessionCache = new Map<string, any>()

async function getSession(modelPath: string): Promise<{ ort: any; session: any }> {
  const ort = await loadOrt()
  let session = sessionCache.get(modelPath)
  if (!session) {
    session = await ort.InferenceSession.create(modelPath, { executionProviders: ['cpu'] })
    sessionCache.set(modelPath, session)
  }
  return { ort, session }
}

// ===== 预处理 =====

interface RawImage {
  width: number
  height: number
  data: Uint8Array // RGBA
}

function decodeImage(base64: string): RawImage {
  const bytes = Buffer.from(base64, 'base64')
  if (bytes.length >= 8 && bytes[0] === 0x89 && bytes[1] === 0x50 && bytes[2] === 0x4e && bytes[3] === 0x47) {
    const png = PNG.sync.read(bytes)
    return { width: png.width, height: png.height, data: new Uint8Array(png.data) }
  }
  if (bytes.length >= 2 && bytes[0] === 0xff && bytes[1] === 0xd8) {
    const img = jpeg.decode(bytes, { useTArray: true, maxMemoryUsageInMB: 256 })
    return { width: img.width, height: img.height, data: new Uint8Array(img.data) }
  }
  throw new Error('无法识别的图片格式，仅支持 PNG / JPEG')
}

// RGBA -> RGB
function toRgb(rgba: Uint8Array): Uint8Array {
  const out = new Uint8Array((rgba.length / 4) * 3)
  for (let i = 0, j = 0; i < rgba.length; i += 4, j += 3) {
    out[j] = rgba[i]
    out[j + 1] = rgba[i + 1]
    out[j + 2] = rgba[i + 2]
  }
  return out
}

interface LetterboxResult {
  data: Float32Array // imgsz*imgsz*3, 0..1 RGB
  scale: number
  padX: number
  padY: number
}

// Ultralytics 风格 letterbox：等比缩放 + 灰色(114)填充到 imgsz x imgsz
function letterbox(srcRgb: Uint8Array, sw: number, sh: number, imgsz: number): LetterboxResult {
  const scale = Math.min(imgsz / sw, imgsz / sh)
  const nw = Math.max(1, Math.round(sw * scale))
  const nh = Math.max(1, Math.round(sh * scale))
  const padX = Math.round((imgsz - nw) / 2)
  const padY = Math.round((imgsz - nh) / 2)
  const out = new Float32Array(imgsz * imgsz * 3)
  out.fill(114 / 255)
  for (let y = 0; y < nh; y++) {
    const sy = Math.min(sh - 1, Math.round(y / scale))
    for (let x = 0; x < nw; x++) {
      const sx = Math.min(sw - 1, Math.round(x / scale))
      const si = (sy * sw + sx) * 3
      const di = ((padY + y) * imgsz + (padX + x)) * 3
      out[di] = srcRgb[si] / 255
      out[di + 1] = srcRgb[si + 1] / 255
      out[di + 2] = srcRgb[si + 2] / 255
    }
  }
  return { data: out, scale, padX, padY }
}

// 把 letterbox 坐标映射回原图并裁剪到图像范围
function clampBox(x1: number, y1: number, x2: number, y2: number, sw: number, sh: number) {
  const x = Math.max(0, Math.min(sw - 1, Math.round(x1)))
  const y = Math.max(0, Math.min(sh - 1, Math.round(y1)))
  const w = Math.max(1, Math.min(sw, Math.round(x2)) - x)
  const h = Math.max(1, Math.min(sh, Math.round(y2)) - y)
  return { x, y, width: w, height: h }
}

// ===== 推理 =====

export async function detectYolo(
  base64Image: string,
  opts: YoloOptions,
): Promise<YoloDetection[]> {
  const raw = decodeImage(base64Image)
  const imgsz = Math.max(32, Math.round(opts.imgsz ?? 640))
  const conf = opts.conf ?? 0.25
  const { data, scale, padX, padY } = letterbox(toRgb(raw.data), raw.width, raw.height, imgsz)

  const { ort, session } = await getSession(resolveModelPath(opts.model))
  const inputName = session.inputNames[0]
  const feed: Record<string, any> = {}
  feed[inputName] = new ort.Tensor('float32', data, [1, 3, imgsz, imgsz])
  const outputs = await session.run(feed)
  const outName = session.outputNames[0]
  const tensor = outputs[outName]
  const dims = tensor.dims
  const vals = tensor.data as Float32Array

  const detections: YoloDetection[] = []

  // 端到端输出 (1, 300, 6) → [x1, y1, x2, y2, conf, class_id]
  if (dims.length === 3 && dims[2] === 6) {
    const n = dims[1] ?? 0
    for (let i = 0; i < n; i++) {
      const o = i * 6
      const score = vals[o + 4]
      if (!(score >= conf)) continue
      const classId = Math.round(vals[o + 5])
      const x1 = (vals[o + 0] - padX) / scale
      const y1 = (vals[o + 1] - padY) / scale
      const x2 = (vals[o + 2] - padX) / scale
      const y2 = (vals[o + 3] - padY) / scale
      const box = clampBox(x1, y1, x2, y2, raw.width, raw.height)
      detections.push({
        classId,
        className: cocoClassName(classId),
        score: Math.round(score * 10000) / 10000,
        ...box,
      })
    }
    return detections.sort((a, b) => b.score - a.score)
  }

  // 传统输出 (1, 4+nc, 8400) → [cx, cy, w, h, scores...]，需要解析 + NMS
  if (dims.length === 3) {
    const numPreds = dims[2]
    const channels = dims[1]
    const nc = channels - 4
    const rawBoxes: Array<{ x1: number; y1: number; x2: number; y2: number; classId: number; score: number }> = []
    for (let p = 0; p < numPreds; p++) {
      const cx = vals[p]
      const cy = vals[numPreds + p]
      const w = vals[numPreds * 2 + p]
      const h = vals[numPreds * 3 + p]
      let bestScore = -1
      let bestCls = 0
      for (let c = 0; c < nc; c++) {
        const s = vals[numPreds * (4 + c) + p]
        if (s > bestScore) {
          bestScore = s
          bestCls = c
        }
      }
      if (bestScore < conf) continue
      const x1 = (cx - w / 2 - padX) / scale
      const y1 = (cy - h / 2 - padY) / scale
      const x2 = (cx + w / 2 - padX) / scale
      const y2 = (cy + h / 2 - padY) / scale
      rawBoxes.push({ x1, y1, x2, y2, classId: bestCls, score: bestScore })
    }
    return nms(rawBoxes, opts.iou ?? 0.45, raw.width, raw.height)
  }

  throw new Error(`无法识别的 YOLO 输出维度: ${JSON.stringify(dims)}`)
}

function iou(a: { x1: number; y1: number; x2: number; y2: number }, b: { x1: number; y1: number; x2: number; y2: number }): number {
  const ix = Math.max(0, Math.min(a.x2, b.x2) - Math.max(a.x1, b.x1))
  const iy = Math.max(0, Math.min(a.y2, b.y2) - Math.max(a.y1, b.y1))
  const inter = ix * iy
  const ua = (a.x2 - a.x1) * (a.y2 - a.y1) + (b.x2 - b.x1) * (b.y2 - b.y1) - inter
  return ua <= 0 ? 0 : inter / ua
}

function nms(
  boxes: Array<{ x1: number; y1: number; x2: number; y2: number; classId: number; score: number }>,
  iouThresh: number,
  sw: number,
  sh: number,
): YoloDetection[] {
  const sorted = [...boxes].sort((a, b) => b.score - a.score)
  const picked: typeof boxes = []
  while (sorted.length > 0) {
    const first = sorted.shift()!
    picked.push(first)
    for (let i = sorted.length - 1; i >= 0; i--) {
      if (sorted[i].classId === first.classId && iou(sorted[i], first) > iouThresh) {
        sorted.splice(i, 1)
      }
    }
  }
  return picked.map((b) => {
    const box = clampBox(b.x1, b.y1, b.x2, b.y2, sw, sh)
    return {
      classId: b.classId,
      className: cocoClassName(b.classId),
      score: Math.round(b.score * 10000) / 10000,
      ...box,
    }
  })
}
