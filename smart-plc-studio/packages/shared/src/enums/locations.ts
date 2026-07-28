// 位置类型枚举

// 位置类型
export enum LocationType {
  ConfNode = 'ConfNode',
  Module = 'Module',
  Group = 'Group',
  VarInput = 'VarInput',
  VarOutput = 'VarOutput',
  VarMemory = 'VarMemory'
}

// 位置类型中文名
export const LocationTypeNames: Record<LocationType, string> = {
  [LocationType.ConfNode]: '配置节点',
  [LocationType.Module]: '模块',
  [LocationType.Group]: '组',
  [LocationType.VarInput]: '输入变量',
  [LocationType.VarOutput]: '输出变量',
  [LocationType.VarMemory]: '内存变量'
}

// 地址前缀映射
export const AddressPrefixes: Record<LocationType, string> = {
  [LocationType.ConfNode]: '%I',
  [LocationType.Module]: '%I',
  [LocationType.Group]: '%M',
  [LocationType.VarInput]: '%I',
  [LocationType.VarOutput]: '%Q',
  [LocationType.VarMemory]: '%M'
}

// 位置分隔符
export const LocationSeparator = '.'

// 地址格式正则表达式
export const AddressPattern = /^(%%[IQM](?:[XBWDL])?)([0-9]+)((?:\.[0-9]+)*)$/
