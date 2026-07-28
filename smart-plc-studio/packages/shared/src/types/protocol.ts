// 通信协议类型定义

// WebSocket 事件类型
export enum WsEvent {
  // 项目事件
  ProjectOpen = 'project:open',
  ProjectClose = 'project:close',
  ProjectSave = 'project:save',
  ProjectModified = 'project:modified',

  // PLC 运行时事件
  RuntimeConnect = 'runtime:connect',
  RuntimeDisconnect = 'runtime:disconnect',
  RuntimeStart = 'runtime:start',
  RuntimeStop = 'runtime:stop',
  RuntimePause = 'runtime:pause',
  RuntimeStatus = 'runtime:status',

  // 调试事件
  DebugStart = 'debug:start',
  DebugStop = 'debug:stop',
  DebugStep = 'debug:step',
  DebugPause = 'debug:pause',
  DebugResume = 'debug:resume',
  DebugBreakpoint = 'debug:breakpoint',
  DebugVariableUpdate = 'debug:variable:update',
  DebugVariableForce = 'debug:variable:force',
  DebugVariableUnforce = 'debug:variable:unforce',

  // 变量监控
  VariableWatch = 'variable:watch',
  VariableUpdate = 'variable:update',

  // 日志
  LogMessage = 'log:message',
  LogClear = 'log:clear',

  // 仿真
  SimStart = 'sim:start',
  SimStop = 'sim:stop',
  SimData = 'sim:data',

  // 错误
  Error = 'error'
}

// 运行时状态
export enum RuntimeStatus {
  Disconnected = 'disconnected',
  Connected = 'connected',
  Started = 'started',
  Stopped = 'stopped',
  Running = 'running',
  Paused = 'paused',
  Error = 'error'
}

// 调试状态
export enum DebugStatus {
  Disconnected = 'disconnected',
  Connected = 'connected',
  Running = 'running',
  Paused = 'paused',
  Stepping = 'stepping'
}

// PLC 运行时状态
export enum PlcStatus {
  Idle = 0,
  Started = 1,
  Stopped = 2,
  Running = 3,
  Paused = 4,
  Error = 5
}

// WebSocket 消息
export interface WsMessage<T = unknown> {
  event: WsEvent
  data: T
  timestamp: number
}

// 变量更新消息
export interface VariableUpdateMessage {
  path: string
  value: unknown
  quality: 'good' | 'bad' | 'uncertain'
}

// 调试变量
export interface DebugVariable {
  path: string
  name: string
  value: unknown
  type: string
  forced: boolean
  forceValue?: unknown
}

// 断点
export interface Breakpoint {
  id: string
  path: string
  line: number
  column?: number
  enabled: boolean
  hitCount?: number
}
