// 工具函数

/**
 * 生成唯一 ID
 */
export function generateId(): string {
  return `${Date.now()}-${Math.random().toString(36).substring(2, 9)}`
}

/**
 * 格式化日期时间
 */
export function formatDateTime(date: Date = new Date()): string {
  return date.toISOString().replace('T', ' ').substring(0, 19)
}

/**
 * 延迟执行
 */
export function delay(ms: number): Promise<void> {
  return new Promise(resolve => setTimeout(resolve, ms))
}

/**
 * 防抖函数
 */
export function debounce<T extends (...args: unknown[]) => unknown>(
  fn: T,
  wait: number
): (...args: Parameters<T>) => void {
  let timeout: ReturnType<typeof setTimeout> | null = null
  return (...args: Parameters<T>) => {
    if (timeout) {
      clearTimeout(timeout)
    }
    timeout = setTimeout(() => fn(...args), wait)
  }
}

/**
 * 节流函数
 */
export function throttle<T extends (...args: unknown[]) => unknown>(
  fn: T,
  limit: number
): (...args: Parameters<T>) => void {
  let inThrottle = false
  return (...args: Parameters<T>) => {
    if (!inThrottle) {
      fn(...args)
      inThrottle = true
      setTimeout(() => (inThrottle = false), limit)
    }
  }
}

/**
 * 深拷贝
 */
export function deepClone<T>(obj: T): T {
  return JSON.parse(JSON.stringify(obj))
}

/**
 * 安全解析 JSON
 */
export function safeJsonParse<T>(json: string, defaultValue: T): T {
  try {
    return JSON.parse(json) as T
  } catch {
    return defaultValue
  }
}

/**
 * 路径工具
 */
export function dirname(path: string): string {
  const parts = path.replace(/\\/g, '/').split('/')
  parts.pop()
  return parts.join('/')
}

export function basename(path: string, ext?: string): string {
  const parts = path.replace(/\\/g, '/').split('/')
  let name = parts.pop() || ''
  if (ext && name.endsWith(ext)) {
    name = name.slice(0, -ext.length)
  }
  return name
}

export function extname(path: string): string {
  const name = basename(path)
  const dotIndex = name.lastIndexOf('.')
  return dotIndex > 0 ? name.slice(dotIndex) : ''
}

export function join(...parts: string[]): string {
  return parts
    .map(p => p.replace(/\\/g, '/'))
    .join('/')
    .replace(/\/+/g, '/')
}

/**
 * 变量名验证
 */
export function isValidVariableName(name: string): boolean {
  return /^[a-zA-Z_][a-zA-Z0-9_]*$/.test(name)
}

/**
 * POU 名称验证
 */
export function isValidPOUName(name: string): boolean {
  return /^[a-zA-Z_][a-zA-Z0-9_]*$/.test(name) && name.length <= 64
}

/**
 * 地址解析
 */
export function parseAddress(address: string): {
  prefix: string
  leading: number
  bits: string
} | null {
  const match = address.match(/^(%%[IQM](?:[XBWDL])?)([0-9]+)((?:\.[0-9]+)*)$/)
  if (!match) return null
  return {
    prefix: match[1],
    leading: parseInt(match[2], 10),
    bits: match[3]
  }
}

/**
 * 创建空变量表
 */
export function createEmptyVarTable() {
  return {
    inputVars: [],
    outputVars: [],
    inOutVars: [],
    localVars: [],
    tempVars: [],
    globalVars: [],
    externalVars: [],
    accessVars: []
  }
}
