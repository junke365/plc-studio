import { defineStore } from "pinia";
import { ref } from "vue";
import type { DebugVariable, Breakpoint } from "@smart-plc/shared";
import { RuntimeStatus, DebugStatus } from "@smart-plc/shared";

export const useDebugStore = defineStore("debug", () => {
  // 运行时状态
  const runtimeStatus = ref<RuntimeStatus>(RuntimeStatus.Disconnected);
  const debugStatus = ref<DebugStatus>(DebugStatus.Disconnected);
  const taskTime = ref<number>(0);

  // 连接信息
  const host = ref<string>("127.0.0.1");
  const port = ref<number>(3000);

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

  // 连接到运行时
  function connect(hostAddr: string, portNum: number) {
    host.value = hostAddr;
    port.value = portNum;
    runtimeStatus.value = RuntimeStatus.Connected;
  }

  // 断开连接
  function disconnect() {
    runtimeStatus.value = RuntimeStatus.Disconnected;
    debugStatus.value = DebugStatus.Disconnected;
    watchVariables.value = [];
  }

  // 开始调试
  function startDebug() {
    debugStatus.value = DebugStatus.Running;
    addLog("info", "调试已启动");
  }

  // 停止调试
  function stopDebug() {
    debugStatus.value = DebugStatus.Disconnected;
    addLog("info", "调试已停止");
  }

  // 暂停调试
  function pauseDebug() {
    debugStatus.value = DebugStatus.Paused;
    addLog("info", "调试已暂停");
  }

  // 恢复调试
  function resumeDebug() {
    debugStatus.value = DebugStatus.Running;
    addLog("info", "调试已恢复");
  }

  // 添加监控变量
  function addWatchVariable(variable: DebugVariable) {
    const existing = watchVariables.value.find((v) => v.path === variable.path);
    if (!existing) {
      watchVariables.value.push(variable);
    }
  }

  // 移除监控变量
  function removeWatchVariable(path: string) {
    const index = watchVariables.value.findIndex((v) => v.path === path);
    if (index >= 0) {
      watchVariables.value.splice(index, 1);
    }
  }

  // 更新变量值
  function updateVariableValue(path: string, value: unknown) {
    const variable = watchVariables.value.find((v) => v.path === path);
    if (variable) {
      variable.value = value;
    }
  }

  // 添加断点
  function addBreakpoint(breakpoint: Breakpoint) {
    const existing = breakpoints.value.find(
      (b) => b.path === breakpoint.path && b.line === breakpoint.line,
    );
    if (!existing) {
      breakpoints.value.push(breakpoint);
    }
  }

  // 移除断点
  function removeBreakpoint(id: string) {
    const index = breakpoints.value.findIndex((b) => b.id === id);
    if (index >= 0) {
      breakpoints.value.splice(index, 1);
    }
  }

  // 切换断点状态
  function toggleBreakpoint(id: string) {
    const breakpoint = breakpoints.value.find((b) => b.id === id);
    if (breakpoint) {
      breakpoint.enabled = !breakpoint.enabled;
    }
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
