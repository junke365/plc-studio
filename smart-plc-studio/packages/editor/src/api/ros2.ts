// ROS 2 调试后端 API
// 开发模式由 Vite 代理转发；Electron 打包后页面以 file:// 加载，
// 此时直接连接本地 Fastify 后端 (127.0.0.1:3000)
const isHttpPage =
  window.location.protocol === "http:" || window.location.protocol === "https:";
const API_ORIGIN = isHttpPage ? "" : "http://127.0.0.1:3000";
const API_BASE = `${API_ORIGIN}/api/ros2`;
const WS_ORIGIN = isHttpPage ? `ws://${window.location.host}` : "ws://127.0.0.1:3000";
export const ROS2_WS_URL = `${WS_ORIGIN}/ws/ros2`;

export type Ros2Strategy = "rclnodejs" | "cli" | "bridge" | "none";

export interface Ros2Status {
  strategy: Ros2Strategy;
  rosDistro: string | null;
  rosPrefix: string | null;
  ros2Cli: boolean;
  rclnodejs: boolean;
  bridgeUrl: string | null;
  description: string;
}

export interface Ros2Topic {
  name: string;
  types: string[];
}

async function getJson<T = any>(path: string): Promise<T> {
  try {
    const res = await fetch(`${API_BASE}${path}`);
    return await res.json();
  } catch (err: any) {
    return { success: false, error: err?.message ?? String(err) } as any;
  }
}

export async function getRos2Status(): Promise<Ros2Status> {
  const r = await getJson<{ success: boolean; data: Ros2Status }>("/status");
  return r.data;
}

export async function getRos2Nodes() {
  return getJson("/nodes");
}

export async function getRos2Topics() {
  return getJson("/topics");
}

export async function getRos2Services() {
  return getJson("/services");
}

export async function getRos2Actions() {
  return getJson("/actions");
}

export async function getRos2TopicType(topic: string) {
  return getJson(`/topic-type?topic=${encodeURIComponent(topic)}`);
}

export async function ros2Publish(
  topic: string,
  type: string,
  message: Record<string, unknown>,
): Promise<{ success: boolean; error?: string }> {
  try {
    const res = await fetch(`${API_BASE}/publish`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ topic, type, message }),
    });
    return await res.json();
  } catch (err: any) {
    return { success: false, error: err?.message ?? String(err) };
  }
}

export async function ros2Call(
  service: string,
  type: string,
  request: Record<string, unknown>,
): Promise<{ success: boolean; data?: string; error?: string }> {
  try {
    const res = await fetch(`${API_BASE}/call`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ service, type, request }),
    });
    return await res.json();
  } catch (err: any) {
    return { success: false, error: err?.message ?? String(err) };
  }
}
