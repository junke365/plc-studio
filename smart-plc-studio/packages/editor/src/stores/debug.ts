import { defineStore } from "pinia";
import { ref } from "vue";
import type { DebugVariable, Breakpoint, WsMessage } from "@smart-plc/shared";
import { RuntimeStatus, DebugStatus } from "@smart-plc/shared";

export const useDebugStore = defineStore("debug", () => {
  // 运行时状态
  const runtimeStatus = ref<RuntimeStatus>(RuntimeStatus.Disconnected);
  const debugStatus = ref<DebugStatus>(DebugStatus.Disconnected);
  const taskTime = ref<number>(0);

  // 连接信息
  const host = ref<string>("127.0.0.1");
  const port = ref<number>(3000);

  // WebSocket 连接
  let ws: WebSocket | null = null;
  let reconnectTimer: ReturnType<typeof setTimeout> | null = null;

  // 监控变量
  const watchVariables = ref<DebugVariable[]>([]);

  // 断点
  const breakpoints = ref<Breakpoint[]>([]);

  // 日志
  const logs = ref<
    Array<{
      timestamp: Date;
      level: "info" | "warning" | "error" | "debug";
      message: string;
    }>
  >([]);

  // 发送 WebSocket 消息
  function send(event: string, payload: Record<string, unknown> = {}) {
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ event, payload }));
    } else {
      addLog("warning", `WS 未连接，无法发送: ${event}`);
    }
  }

  // 处理接收到的 WS 消息
  function handleWsMessage(data: WsMessage) {
    const { event, data: payload } = data;
    switch (event) {
      case "runtime:status":
        runtimeStatus.value = payload.status as RuntimeStatus;
        taskTime.value = (payload as any).taskTime ?? 0;
        break;

      case "started":
        debugStatus.value = DebugStatus.Running;
        addLog("info", "调试已启动");
        break;

      case "stopped":
        debugStatus.value = DebugStatus.Disconnected;
        addLog("info", "调试已停止");
        break;

      case "paused":
        debugStatus.value = DebugStatus.Paused;
        addLog("info", "调试已暂停");
        break;

      case "resumed":
        debugStatus.value = DebugStatus.Running;
        addLog("info", "调试已恢复");
        break;

      case "stepped":
        addLog("debug", "单步执行完成");
        break;

      case "debug:variable:update": {
        const pl = payload as any;
        if (pl.variables) {
          // 批量更新
          const vars = pl.variables as DebugVariable[];
          for (const v of vars) {
            const existing = watchVariables.value.find((wv) => wv.path === v.path);
            if (existing) {
              existing.value = v.value;
            }
          }
        } else if (pl.path) {
          // 单个变量更新
          const existing = watchVariables.value.find((wv) => wv.path === pl.path);
          if (existing) {
            existing.value = pl.value;
          }
        }
        break;
      }

      case "variable:update": {
        const pl = payload as any;
        const existing = watchVariables.value.find((wv) => wv.path === pl.path);
        if (existing) {
          existing.value = pl.value;
        }
        break;
      }

      case "debug:breakpoint": {
        const pl = payload as any;
        if (pl.action === "added" && pl.breakpoint) {
          addBreakpointLocal(pl.breakpoint);
        } else if (pl.action === "removed" && pl.id) {
          removeBreakpointLocal(pl.id);
        } else if (pl.action === "toggled" && pl.breakpoint) {
          toggleBreakpointLocal(pl.breakpoint.id);
        }
        break;
      }

      case "log:message":
        addLog((payload as any).level || "info", (payload as any).message || "");
        break;

      case "error":
        addLog("error", (payload as any).message || "未知错误");
        break;
    }
  }

  // 连接到运行时
  async function connect(hostAddr: string, portNum: number) {
    host.value = hostAddr;
    port.value = portNum;

    try {
      const url = `ws://${hostAddr}:${portNum}/ws/debug`;
      ws = new WebSocket(url);

      ws.onopen = () => {
        runtimeStatus.value = RuntimeStatus.Connected;
        addLog("info", `已连接到 ${hostAddr}:${portNum}`);
        // 获取运行时状态
        send("getStatus");
      };

      ws.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          handleWsMessage(data);
        } catch {
          addLog("error", "收到无效消息");
        }
      };

      ws.onclose = () => {
        runtimeStatus.value = RuntimeStatus.Disconnected;
        debugStatus.value = DebugStatus.Disconnected;
        ws = null;
        addLog("warning", "连接已断开");
      };

      ws.onerror = () => {
        addLog("error", "WebSocket 连接失败");
        runtimeStatus.value = RuntimeStatus.Disconnected;
      };
    } catch (e) {
      addLog("error", `连接失败: ${(e as Error).message}`);
      runtimeStatus.value = RuntimeStatus.Disconnected;
    }
  }

  // 断开连接
  function disconnect() {
    if (ws) {
      ws.close();
      ws = null;
    }
    if (reconnectTimer) {
      clearTimeout(reconnectTimer);
      reconnectTimer = null;
    }
    runtimeStatus.value = RuntimeStatus.Disconnected;
    debugStatus.value = DebugStatus.Disconnected;
    watchVariables.value = [];
    addLog("info", "已断开连接");
  }

  // 开始调试
  function startDebug() {
    send("start", { projectPath: "" });
  }

  // 停止调试
  function stopDebug() {
    send("stop");
  }

  // 暂停调试
  function pauseDebug() {
    send("pause");
  }

  // 恢复调试
  function resumeDebug() {
    send("resume");
  }

  // 单步执行
  function stepDebug() {
    send("step");
  }

  // 读取变量
  function readVariable(path: string) {
    send("readVariable", { path });
  }

  // 写入变量
  function writeVariable(path: string, value: unknown) {
    send("writeVariable", { path, value });
  }

  // 读取所有变量
  function readAllVariables() {
    send("readAllVariables");
  }

  // 添加监控变量（本地 + 同步到后端）
  function addWatchVariable(variable: DebugVariable) {
    const existing = watchVariables.value.find((v) => v.path === variable.path);
    if (!existing) {
      watchVariables.value.push(variable);
      // 通知后端注册该变量
      send("readVariable", { path: variable.path });
    }
  }

  // 移除监控变量
  function removeWatchVariable(path: string) {
    const index = watchVariables.value.findIndex((v) => v.path === path);
    if (index >= 0) {
      watchVariables.value.splice(index, 1);
    }
  }

  // 更新变量值（本地）
  function updateVariableValue(path: string, value: unknown) {
    const variable = watchVariables.value.find((v) => v.path === path);
    if (variable) {
      variable.value = value;
    }
  }

  // 添加断点（本地）
  function addBreakpointLocal(bp: Breakpoint) {
    const existing = breakpoints.value.find(
      (b) => b.path === bp.path && b.line === bp.line,
    );
    if (!existing) {
      breakpoints.value.push(bp);
    }
  }

  // 移除断点（本地）
  function removeBreakpointLocal(id: string) {
    const index = breakpoints.value.findIndex((b) => b.id === id);
    if (index >= 0) {
      breakpoints.value.splice(index, 1);
    }
  }

  // 切换断点（本地）
  function toggleBreakpointLocal(id: string) {
    const breakpoint = breakpoints.value.find((b) => b.id === id);
    if (breakpoint) {
      breakpoint.enabled = !breakpoint.enabled;
    }
  }

  // 添加断点（同步到后端）
  function addBreakpoint(breakpoint: Breakpoint) {
    addBreakpointLocal(breakpoint);
    send("addBreakpoint", breakpoint as unknown as Record<string, unknown>);
  }

  // 移除断点（同步到后端）
  function removeBreakpoint(id: string) {
    removeBreakpointLocal(id);
    send("removeBreakpoint", { id });
  }

  // 切换断点状态（同步到后端）
  function toggleBreakpoint(id: string) {
    toggleBreakpointLocal(id);
    send("toggleBreakpoint", { id });
  }

  // 添加日志
  function addLog(
    level: "info" | "warning" | "error" | "debug",
    message: string,
  ) {
    logs.value.push({
      timestamp: new Date(),
      level,
      message,
    });
  }

  // 清空日志
  function clearLogs() {
    logs.value = [];
  }

  return {
    runtimeStatus,
    debugStatus,
    taskTime,
    host,
    port,
    watchVariables,
    breakpoints,
    logs,
    connect,
    disconnect,
    startDebug,
    stopDebug,
    pauseDebug,
    resumeDebug,
    stepDebug,
    readVariable,
    writeVariable,
    readAllVariables,
    addWatchVariable,
    removeWatchVariable,
    updateVariableValue,
    addBreakpoint,
    removeBreakpoint,
    toggleBreakpoint,
    addLog,
    clearLogs,
  };
});
