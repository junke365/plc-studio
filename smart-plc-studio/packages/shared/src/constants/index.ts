// 常量定义

// 应用信息
export const APP_NAME = 'Smart PLC Studio'
export const APP_VERSION = '1.0.0'
export const APP_DESCRIPTION = '智能 PLC IDE - IEC 61131-3 集成开发环境'

// 默认配置
export const DEFAULT_PORT = 3000
export const DEFAULT_HOST = '127.0.0.1'
export const DEFAULT_EDITOR_PORT = 5173

// PLC 配置
export const MAX_UNDO_BUFFER = 20
export const MAX_VARIABLES = 10000
export const MAX_POU_NAME_LENGTH = 64

// 画布配置
export const CANVAS_GRID_SIZE = 10
export const CANVAS_ZOOM_MIN = 0.1
export const CANVAS_ZOOM_MAX = 5.0
export const CANVAS_ZOOM_STEP = 0.1
export const CANVAS_DEFAULT_WIDTH = 1200
export const CANVAS_DEFAULT_HEIGHT = 800

// 文件扩展名
export const PLC_FILE_EXTENSION = '.plcproj'
export const PLC_OPEN_EXTENSION = '.xml'
export const BACKUP_EXTENSION = '.bak'
export const TEMP_EXTENSION = '.tmp'

// 日志级别
export const LOG_LEVELS = {
  DEBUG: 0,
  INFO: 1,
  WARNING: 2,
  ERROR: 3,
  CRITICAL: 4
}

// 运行时默认配置
export const RUNTIME_DEFAULTS = {
  scanTime: 10,        // 扫描周期（ms）
  maxRetries: 3,       // 最大重试次数
  timeout: 5000,       // 超时时间（ms）
  heartbeatInterval: 1000  // 心跳间隔（ms）
}
