import { loadOpenCV } from '@opencvjs/node'
import { PNG } from 'pngjs'
import jpeg from 'jpeg-js'
import { detectYolo, type YoloDetection } from './yolo.service.js'

// ===== 类型定义 =====

export type MatchMethod =
  | 'sqdiff'
  | 'sqdiff_normed'
  | 'ccorr'
  | 'ccorr_normed'
  | 'ccoeff'
  | 'ccoeff_normed'

export interface MatchTemplateResult {
  x: number
  y: number
  width: number
  height: number
  score: number
}

export interface ContourResult {
  x: number
  y: number
  width: number
  height: number
  area: number
  points: number
}

export type VisionOp =
  | { type: 'grayscale' }
  | { type: 'blur'; ksize?: number; sigma?: number }
  | {
      type: 'threshold'
      thresh?: number
      maxval?: number
      invert?: boolean
      kind?: 'binary' | 'binary_inv' | 'trunc' | 'tozero' | 'tozero_inv' | 'otsu'
    }
  | { type: 'canny'; low?: number; high?: number; aperture?: number }
  | { type: 'resize'; width?: number; height?: number }
  | { type: 'rotate'; angle?: 90 | 180 | 270 }
  | { type: 'cvtColor'; code?: 'gray' | 'hsv' | 'lab' | 'luv' | 'ycc' }
  | { type: 'flip'; flipCode?: 0 | 1 | -1 }
  | { type: 'crop'; x?: number; y?: number; width?: number; height?: number }
  | { type: 'erode'; ksize?: number; iterations?: number }
  | { type: 'dilate'; ksize?: number; iterations?: number }
  | {
      type: 'morph'
      op?: 'open' | 'close' | 'gradient' | 'tophat' | 'blackhat'
      ksize?: number
      iterations?: number
    }
  | { type: 'medianBlur'; ksize?: number }
  | { type: 'bilateral'; d?: number; sigmaColor?: number; sigmaSpace?: number }
  | { type: 'adaptiveThreshold'; blockSize?: number; c?: number; invert?: boolean }
  | { type: 'sobel'; dx?: number; dy?: number; ksize?: number }
  | { type: 'laplacian'; ksize?: number }
  | { type: 'equalizeHist' }
  | { type: 'pyrDown' }
  | { type: 'pyrUp' }
  | { type: 'brightnessContrast'; alpha?: number; beta?: number }
  | {
      type: 'inRange'
      low: number[]
      high: number[]
      space?: 'bgr' | 'hsv'
    }
  | { type: 'matchTemplate'; template: string; method?: MatchMethod; threshold?: number }
  | {
      type: 'contours'
      minArea?: number
      mode?: 'external' | 'list' | 'tree'
      approx?: 'simple' | 'none' | 'l1'
    }
  | { type: 'houghLinesP'; threshold?: number; minLineLength?: number; maxLineGap?: number }
  | {
      type: 'houghCircles'
      minDist?: number
      param1?: number
      param2?: number
      minRadius?: number
      maxRadius?: number
    }
  | { type: 'yolo'; model?: string; conf?: number; iou?: number; imgsz?: number }

export interface ProcessRequest {
  image: string // base64 png/jpg
  ops?: VisionOp[]
  // locate 便捷接口参数
  template?: string
  method?: MatchMethod
  threshold?: number
}

export interface ProcessResponse {
  width: number
  height: number
  image: string // base64 png（含轮廓/匹配框叠加）
  results: Record<string, any>
}

// ===== OpenCV 单例 =====

let cvPromise: Promise<any> | null = null

export function getCv(): Promise<any> {
  if (!cvPromise) {
    cvPromise = loadOpenCV()
  }
  return cvPromise
}

// ===== 工具 =====
// OpenCV 5 的 WASM 构建暂未包含 imgcodecs 的 imencode/imdecode，
// 因此用 pngjs / jpeg-js 做 PNG/JPEG 编解码，OpenCV 负责像素运算。

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

function decodeToMat(cv: any, base64: string): any {
  const raw = decodeImage(base64)
  const rgba = cv.matFromImageData({
    data: new Uint8ClampedArray(raw.data),
    width: raw.width,
    height: raw.height,
  })
  const mat = new cv.Mat()
  cv.cvtColor(rgba, mat, cv.COLOR_RGBA2BGR)
  rgba.delete()
  return mat
}

function encodeMatToBase64(cv: any, mat: any, colorSpace: ColorSpace): string {
  const rgba = new cv.Mat()
  if (mat.channels() === 1) {
    cv.cvtColor(mat, rgba, cv.COLOR_GRAY2RGBA)
  } else if (colorSpace === 'bgr') {
    cv.cvtColor(mat, rgba, cv.COLOR_BGR2RGBA)
  } else {
    // 非 BGR 色彩空间先转回 BGR 再编码，保证预览颜色正确
    const backCode =
      colorSpace === 'hsv' ? cv.COLOR_HSV2BGR
      : colorSpace === 'lab' ? cv.COLOR_Lab2BGR
      : colorSpace === 'luv' ? cv.COLOR_Luv2BGR
      : cv.COLOR_YCrCb2BGR
    const bgr = new cv.Mat()
    cv.cvtColor(mat, bgr, backCode)
    cv.cvtColor(bgr, rgba, cv.COLOR_BGR2RGBA)
    bgr.delete()
  }
  const w = mat.cols
  const h = mat.rows
  const data = new Uint8ClampedArray(rgba.data)
  rgba.delete()
  const png = new PNG({ width: w, height: h })
  png.data = Buffer.from(data)
  return PNG.sync.write(png).toString('base64')
}

type ColorSpace = 'bgr' | 'gray' | 'hsv' | 'lab' | 'luv' | 'ycc'

function matchMethodCode(cv: any, method?: MatchMethod): number {
  switch (method ?? 'ccoeff_normed') {
    case 'sqdiff': return cv.TM_SQDIFF
    case 'sqdiff_normed': return cv.TM_SQDIFF_NORMED
    case 'ccorr': return cv.TM_CCORR
    case 'ccorr_normed': return cv.TM_CCORR_NORMED
    case 'ccoeff': return cv.TM_CCOEFF
    case 'ccoeff_normed': return cv.TM_CCOEFF_NORMED
    default: return cv.TM_CCOEFF_NORMED
  }
}

// ===== 处理管线 =====

export async function processImage(req: ProcessRequest): Promise<ProcessResponse> {
  const cv = await getCv()
  let src = decodeToMat(cv, req.image)
  let colorSpace: ColorSpace = 'bgr'
  const ops = req.ops ?? []
  const measurements: Record<string, any> = {}

  try {
    for (const op of ops) {
      switch (op.type) {
        case 'grayscale': {
          if (src.channels() === 3) {
            const dst = new cv.Mat()
            cv.cvtColor(src, dst, cv.COLOR_BGR2GRAY)
            src.delete(); src = dst
          }
          colorSpace = 'gray'
          break
        }
        case 'blur': {
          const dst = new cv.Mat()
          const ksize = Math.max(1, Math.round(op.ksize ?? 5) | 1)
          cv.GaussianBlur(src, dst, new cv.Size(ksize, ksize), op.sigma ?? 0, op.sigma ?? 0, cv.BORDER_DEFAULT)
          src.delete(); src = dst
          break
        }
        case 'threshold': {
          const dst = new cv.Mat()
          const kind = op.kind ?? 'binary'
          const base =
            kind === 'binary_inv' ? cv.THRESH_BINARY_INV
            : kind === 'trunc' ? cv.THRESH_TRUNC
            : kind === 'tozero' ? cv.THRESH_TOZERO
            : kind === 'tozero_inv' ? cv.THRESH_TOZERO_INV
            : cv.THRESH_BINARY
          const type = kind === 'otsu'
            ? cv.THRESH_BINARY | cv.THRESH_OTSU
            : op.invert ? (base === cv.THRESH_BINARY ? cv.THRESH_BINARY_INV : base) : base
          cv.threshold(src, dst, op.thresh ?? 127, op.maxval ?? 255, type)
          src.delete(); src = dst
          colorSpace = 'gray'
          break
        }
        case 'canny': {
          const dst = new cv.Mat()
          cv.Canny(src, dst, op.low ?? 50, op.high ?? 150, op.aperture ?? 3)
          src.delete(); src = dst
          colorSpace = 'gray'
          break
        }
        case 'resize': {
          const dst = new cv.Mat()
          const w = op.width ?? src.cols
          const h = op.height ?? src.rows
          cv.resize(src, dst, new cv.Size(Math.max(1, w), Math.max(1, h)), 0, 0, cv.INTER_LINEAR)
          src.delete(); src = dst
          break
        }
        case 'rotate': {
          const dst = new cv.Mat()
          const code = op.angle === 90 ? cv.ROTATE_90_CLOCKWISE
            : op.angle === 270 ? cv.ROTATE_90_COUNTERCLOCKWISE
            : cv.ROTATE_180
          cv.rotate(src, dst, code)
          src.delete(); src = dst
          break
        }
        case 'cvtColor': {
          let dst = new cv.Mat()
          const code = op.code ?? 'hsv'
          const srcCh = src.channels()
          const target = code === 'gray' ? 'gray' : code
          if (code === 'gray') {
            if (srcCh === 3) cv.cvtColor(src, dst, cv.COLOR_BGR2GRAY)
            else dst = src.clone()
          } else {
            const convCode =
              code === 'hsv' ? cv.COLOR_BGR2HSV
              : code === 'lab' ? cv.COLOR_BGR2Lab
              : code === 'luv' ? cv.COLOR_BGR2Luv
              : cv.COLOR_BGR2YCrCb
            cv.cvtColor(src, dst, convCode)
          }
          src.delete(); src = dst
          colorSpace = target as ColorSpace
          break
        }
        case 'flip': {
          const dst = new cv.Mat()
          cv.flip(src, dst, op.flipCode ?? 0)
          src.delete(); src = dst
          break
        }
        case 'crop': {
          const w = op.width ?? src.cols
          const h = op.height ?? src.rows
          const x = Math.max(0, Math.min(op.x ?? 0, src.cols - 1))
          const y = Math.max(0, Math.min(op.y ?? 0, src.rows - 1))
          const cw = Math.max(1, Math.min(w, src.cols - x))
          const ch = Math.max(1, Math.min(h, src.rows - y))
          const dst = src.roi(new cv.Rect(x, y, cw, ch))
          src.delete(); src = dst
          break
        }
        case 'erode':
        case 'dilate': {
          const ksize = Math.max(1, Math.round(op.ksize ?? 3) | 1)
          const kernel = cv.getStructuringElement(cv.MORPH_RECT, new cv.Size(ksize, ksize))
          const dst = new cv.Mat()
          if (op.type === 'erode') cv.erode(src, dst, kernel, new cv.Point(-1, -1), op.iterations ?? 1)
          else cv.dilate(src, dst, kernel, new cv.Point(-1, -1), op.iterations ?? 1)
          kernel.delete()
          src.delete(); src = dst
          break
        }
        case 'morph': {
          const ksize = Math.max(1, Math.round(op.ksize ?? 3) | 1)
          const kernel = cv.getStructuringElement(cv.MORPH_RECT, new cv.Size(ksize, ksize))
          const morphOp =
            op.op === 'open' ? cv.MORPH_OPEN
            : op.op === 'close' ? cv.MORPH_CLOSE
            : op.op === 'gradient' ? cv.MORPH_GRADIENT
            : op.op === 'tophat' ? cv.MORPH_TOPHAT
            : cv.MORPH_BLACKHAT
          const dst = new cv.Mat()
          cv.morphologyEx(src, dst, morphOp, kernel, new cv.Point(-1, -1), op.iterations ?? 1)
          kernel.delete()
          src.delete(); src = dst
          break
        }
        case 'medianBlur': {
          const ksize = Math.max(1, Math.round(op.ksize ?? 5) | 1)
          const dst = new cv.Mat()
          cv.medianBlur(src, dst, ksize)
          src.delete(); src = dst
          break
        }
        case 'bilateral': {
          const dst = new cv.Mat()
          cv.bilateralFilter(src, dst, op.d ?? 9, op.sigmaColor ?? 75, op.sigmaSpace ?? 75)
          src.delete(); src = dst
          break
        }
        case 'adaptiveThreshold': {
          const dst = new cv.Mat()
          const gray = src.channels() === 3 ? toGray(cv, src) : src
          const type = op.invert ? cv.THRESH_BINARY_INV : cv.THRESH_BINARY
          cv.adaptiveThreshold(gray, dst, 255, cv.ADAPTIVE_THRESH_GAUSSIAN_C, type, op.blockSize ?? 11, op.c ?? 2)
          if (gray !== src) gray.delete()
          src.delete(); src = dst
          colorSpace = 'gray'
          break
        }
        case 'sobel': {
          const dst = new cv.Mat()
          const gray = src.channels() === 3 ? toGray(cv, src) : src
          cv.Sobel(gray, dst, cv.CV_16S, op.dx ?? 1, op.dy ?? 0, op.ksize ?? 3)
          const abs = new cv.Mat()
          cv.convertScaleAbs(dst, abs)
          if (gray !== src) gray.delete()
          dst.delete()
          src.delete(); src = abs
          colorSpace = 'gray'
          break
        }
        case 'laplacian': {
          const dst = new cv.Mat()
          const gray = src.channels() === 3 ? toGray(cv, src) : src
          cv.Laplacian(gray, dst, cv.CV_16S, op.ksize ?? 3)
          const abs = new cv.Mat()
          cv.convertScaleAbs(dst, abs)
          if (gray !== src) gray.delete()
          dst.delete()
          src.delete(); src = abs
          colorSpace = 'gray'
          break
        }
        case 'equalizeHist': {
          const dst = new cv.Mat()
          const gray = src.channels() === 3 ? toGray(cv, src) : src
          cv.equalizeHist(gray, dst)
          if (gray !== src) gray.delete()
          src.delete(); src = dst
          colorSpace = 'gray'
          break
        }
        case 'pyrDown':
        case 'pyrUp': {
          const dst = new cv.Mat()
          if (op.type === 'pyrDown') cv.pyrDown(src, dst)
          else cv.pyrUp(src, dst)
          src.delete(); src = dst
          break
        }
        case 'brightnessContrast': {
          const dst = new cv.Mat()
          const alpha = op.alpha ?? 1.0
          const beta = op.beta ?? 0
          // dst = saturate(|alpha*src + beta|)
          cv.convertScaleAbs(src, dst, alpha, beta)
          src.delete(); src = dst
          break
        }
        case 'inRange': {
          const dst = new cv.Mat()
          const low = op.low ?? [0, 0, 0]
          const high = op.high ?? [255, 255, 255]
          if (op.space === 'hsv') {
            const hsv = new cv.Mat()
            cv.cvtColor(src, hsv, cv.COLOR_BGR2HSV)
            const lowMat = new cv.Mat(hsv.rows, hsv.cols, cv.CV_8UC3, new cv.Scalar(...low))
            const highMat = new cv.Mat(hsv.rows, hsv.cols, cv.CV_8UC3, new cv.Scalar(...high))
            cv.inRange(hsv, lowMat, highMat, dst)
            lowMat.delete(); highMat.delete(); hsv.delete()
          } else {
            const lowMat = new cv.Mat(src.rows, src.cols, cv.CV_8UC3, new cv.Scalar(...low))
            const highMat = new cv.Mat(src.rows, src.cols, cv.CV_8UC3, new cv.Scalar(...high))
            cv.inRange(src, lowMat, highMat, dst)
            lowMat.delete(); highMat.delete()
          }
          src.delete(); src = dst
          colorSpace = 'gray'
          break
        }
        case 'houghLinesP': {
          const gray = src.channels() === 3 ? toGray(cv, src) : src
          const edges = new cv.Mat()
          cv.Canny(gray, edges, 50, 150)
          const lines = new cv.Mat()
          cv.HoughLinesP(edges, lines, 1, Math.PI / 180, op.threshold ?? 80, op.minLineLength ?? 40, op.maxLineGap ?? 10)
          const l = lines.data32S
          const color = new cv.Scalar(0, 255, 0, 255)
          const list: Array<{ x1: number; y1: number; x2: number; y2: number }> = []
          for (let i = 0; i < lines.rows; i++) {
            const x1 = l[i * 4], y1 = l[i * 4 + 1], x2 = l[i * 4 + 2], y2 = l[i * 4 + 3]
            cv.line(src, new cv.Point(x1, y1), new cv.Point(x2, y2), color, 2)
            list.push({ x1, y1, x2, y2 })
          }
          measurements.lines = list
          edges.delete(); lines.delete()
          if (gray !== src) gray.delete()
          break
        }
        case 'houghCircles': {
          const gray = src.channels() === 3 ? toGray(cv, src) : src
          const blurred = new cv.Mat()
          cv.GaussianBlur(gray, blurred, new cv.Size(5, 5), 1.2, 1.2, cv.BORDER_DEFAULT)
          const circles = new cv.Mat()
          cv.HoughCircles(
            blurred, circles, cv.HOUGH_GRADIENT, 1,
            op.minDist ?? Math.max(20, Math.round(src.rows / 8)),
            op.param1 ?? 100, op.param2 ?? 30,
            op.minRadius ?? 5, op.maxRadius ?? Math.round(src.rows / 2),
          )
          const c = circles.data32F
          const color = new cv.Scalar(0, 255, 0, 255)
          const list: Array<{ x: number; y: number; radius: number }> = []
          for (let i = 0; i < circles.rows; i++) {
            const x = Math.round(c[i * 3]), y = Math.round(c[i * 3 + 1]), r = Math.round(c[i * 3 + 2])
            cv.circle(src, new cv.Point(x, y), r, color, 2)
            list.push({ x, y, radius: r })
          }
          measurements.circles = list
          blurred.delete(); circles.delete()
          if (gray !== src) gray.delete()
          break
        }
        case 'matchTemplate': {
          const templ = decodeToMat(cv, op.template)
          let templGray: any
          let srcGray: any
          if (src.channels() === 3) {
            templGray = new cv.Mat()
            srcGray = new cv.Mat()
            cv.cvtColor(src, srcGray, cv.COLOR_BGR2GRAY)
            cv.cvtColor(templ, templGray, cv.COLOR_BGR2GRAY)
          } else {
            srcGray = src
            templGray = new cv.Mat()
            if (templ.channels() === 3) {
              cv.cvtColor(templ, templGray, cv.COLOR_BGR2GRAY)
            } else {
              templGray = templ.clone()
            }
          }
          const result = new cv.Mat()
          const method = matchMethodCode(cv, op.method)
          cv.matchTemplate(srcGray, templGray, result, method)
          const { minVal, maxVal, minLoc, maxLoc } = cv.minMaxLoc(result)
          const isSq = method === cv.TM_SQDIFF || method === cv.TM_SQDIFF_NORMED
          const loc = isSq ? minLoc : maxLoc
          const score = isSq ? minVal : maxVal
          const matches: MatchTemplateResult[] = [{
            x: loc.x,
            y: loc.y,
            width: templGray.cols,
            height: templGray.rows,
            score: Math.round(score * 10000) / 10000,
          }]
          const threshold = op.threshold ?? (isSq ? -1 : 0.8)
          const good = matches.filter((m) => (isSq ? m.score <= threshold : m.score >= threshold))
          if (good.length > 0) {
            const color = new cv.Scalar(0, 255, 0, 255)
            for (const m of good) {
              cv.rectangle(src, new cv.Point(m.x, m.y), new cv.Point(m.x + m.width, m.y + m.height), color, 2)
            }
          }
          measurements.templateMatches = good
          result.delete(); templ.delete(); templGray.delete()
          if (srcGray !== src) srcGray.delete()
          break
        }
        case 'contours': {
          const srcCopy = src.clone()
          let gray = new cv.Mat()
          if (srcCopy.channels() === 3) {
            cv.cvtColor(srcCopy, gray, cv.COLOR_BGR2GRAY)
          } else {
            gray.delete()
            gray = srcCopy.clone()
          }
          const edges = new cv.Mat()
          cv.threshold(gray, edges, 127, 255, cv.THRESH_BINARY)
          const contours = new cv.MatVector()
          const hierarchy = new cv.Mat()
          const mode =
            op.mode === 'list' ? cv.RETR_LIST
            : op.mode === 'tree' ? cv.RETR_TREE
            : cv.RETR_EXTERNAL
          const approx =
            op.approx === 'l1' ? cv.CHAIN_APPROX_TC89_L1
            : op.approx === 'none' ? cv.CHAIN_APPROX_NONE
            : cv.CHAIN_APPROX_SIMPLE
          cv.findContours(edges, contours, hierarchy, mode, approx)
          const minArea = op.minArea ?? 0
          const list: ContourResult[] = []
          const color = new cv.Scalar(0, 165, 255, 255)
          for (let i = 0; i < contours.size(); i++) {
            const area = cv.contourArea(contours.get(i))
            if (area < minArea) continue
            const rect = cv.boundingRect(contours.get(i))
            list.push({
              x: rect.x, y: rect.y, width: rect.width, height: rect.height,
              area: Math.round(area * 100) / 100,
              points: contours.get(i).rows,
            })
            cv.rectangle(src, new cv.Point(rect.x, rect.y), new cv.Point(rect.x + rect.width, rect.y + rect.height), color, 2)
          }
          measurements.contours = list
          contours.delete(); hierarchy.delete(); edges.delete()
          gray.delete()
          srcCopy.delete()
          break
        }
        case 'yolo': {
          const imgB64 = encodeMatToBase64(cv, src, colorSpace)
          const detections = await detectYolo(imgB64, {
            model: op.model ?? 'yolo26n.onnx',
            conf: op.conf,
            iou: op.iou,
            imgsz: op.imgsz,
          })
          const color = new cv.Scalar(0, 255, 0, 255)
          const labelColor = new cv.Scalar(0, 200, 0, 255)
          for (const d of detections as YoloDetection[]) {
            cv.rectangle(
              src,
              new cv.Point(d.x, d.y),
              new cv.Point(d.x + d.width, d.y + d.height),
              color,
              2,
            )
            const label = `${d.className} ${(d.score * 100).toFixed(0)}%`
            const org = new cv.Point(d.x, Math.max(0, d.y - 4))
            if (d.y >= 12) {
              cv.putText(src, label, org, cv.FONT_HERSHEY_SIMPLEX, 0.5, labelColor, 1, cv.LINE_AA)
            }
          }
          measurements.detections = detections
          colorSpace = 'bgr'
          break
        }
        default:
          throw new Error(`未知的视觉操作: ${(op as any).type}`)
      }
    }

    return {
      width: src.cols,
      height: src.rows,
      image: encodeMatToBase64(cv, src, colorSpace),
      results: measurements,
    }
  } finally {
    src.delete()
  }
}

function toGray(cv: any, mat: any): any {
  const dst = new cv.Mat()
  cv.cvtColor(mat, dst, cv.COLOR_BGR2GRAY)
  return dst
}
