// 视觉后端 API（OpenCV5）
// 开发模式由 Vite 代理转发；Electron 打包后页面以 file:// 加载，
// 此时直接连接本地 Fastify 后端 (127.0.0.1:3000)
const isHttpPage =
  window.location.protocol === "http:" || window.location.protocol === "https:";
const API_ORIGIN = isHttpPage ? "" : "http://127.0.0.1:3000";
const API_BASE = `${API_ORIGIN}/api/vision`;

export type VisionOpType =
  | "grayscale"
  | "blur"
  | "threshold"
  | "canny"
  | "resize"
  | "rotate"
  | "inRange"
  | "matchTemplate"
  | "contours"
  | "yolo";

export interface VisionOp {
  type: VisionOpType;
  [key: string]: any;
}

export async function getVisionVersion(): Promise<any> {
  try {
    const res = await fetch(`${API_BASE}/version`);
    return await res.json();
  } catch (err: any) {
    return { available: false, error: err?.message ?? String(err) };
  }
}

export interface VisionModels {
  models: string[];
  dir: string;
}

export async function getVisionModels(): Promise<VisionModels> {
  try {
    const res = await fetch(`${API_BASE}/models`);
    return await res.json();
  } catch (err: any) {
    return { models: [], dir: "", error: err?.message ?? String(err) } as any;
  }
}

export async function processVisionImage(
  imageBase64: string,
  ops: VisionOp[] = [],
): Promise<{ success: boolean; data?: any; error?: string }> {
  try {
    const res = await fetch(`${API_BASE}/process`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ image: imageBase64, ops }),
    });
    return await res.json();
  } catch (err: any) {
    return { success: false, error: err?.message ?? String(err) };
  }
}
