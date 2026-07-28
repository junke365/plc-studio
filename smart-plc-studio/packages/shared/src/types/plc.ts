// PLC 数据类型定义

export type PlcBool = boolean
export type PlcInt = number
export type PlcDint = number
export type PlcLint = number
export type PlcWord = number
export type PlcDword = number
export type PlcLword = number
export type PlcByte = number
export type PlcSint = number
export type PlcUsint = number
export type PlcUint = number
export type PlcUdint = number
export type PlcUlint = number
export type PlcReal = number
export type PlcLreal = number
export type PlcTime = number
export type PlcDate = Date
export type PlcTod = number
export type PlcDt = Date
export type PlcString = string
export type PlcWstring = string

// IEC 61131-3 基本类型
export enum IecType {
  Bool = 'BOOL',
  Int = 'INT',
  Dint = 'DINT',
  Lint = 'LINT',
  Word = 'WORD',
  Dword = 'DWORD',
  Lword = 'LWORD',
  Byte = 'BYTE',
  Sint = 'SINT',
  Usint = 'USINT',
  Uint = 'UINT',
  Udint = 'UDINT',
  Ulint = 'ULINT',
  Real = 'REAL',
  Lreal = 'LREAL',
  Time = 'TIME',
  Date = 'DATE',
  Tod = 'TOD',
  Dt = 'DT',
  String = 'STRING',
  Wstring = 'WSTRING'
}

// 类型信息
export interface TypeInfo {
  name: string
  baseType?: string
  size: number
  initialValue?: unknown
}

// 类型映射到 TypeScript
export const TypeMapping: Record<string, string> = {
  [IecType.Bool]: 'boolean',
  [IecType.Int]: 'number',
  [IecType.Dint]: 'number',
  [IecType.Lint]: 'number',
  [IecType.Word]: 'number',
  [IecType.Dword]: 'number',
  [IecType.Lword]: 'number',
  [IecType.Byte]: 'number',
  [IecType.Sint]: 'number',
  [IecType.Usint]: 'number',
  [IecType.Uint]: 'number',
  [IecType.Udint]: 'number',
  [IecType.Ulint]: 'number',
  [IecType.Real]: 'number',
  [IecType.Lreal]: 'number',
  [IecType.Time]: 'number',
  [IecType.Date]: 'Date',
  [IecType.Tod]: 'number',
  [IecType.Dt]: 'Date',
  [IecType.String]: 'string',
  [IecType.Wstring]: 'string'
}

// 类型大小（字节）
export const TypeSizes: Record<string, number> = {
  [IecType.Bool]: 1,
  [IecType.Int]: 2,
  [IecType.Dint]: 4,
  [IecType.Lint]: 8,
  [IecType.Word]: 2,
  [IecType.Dword]: 4,
  [IecType.Lword]: 8,
  [IecType.Byte]: 1,
  [IecType.Sint]: 1,
  [IecType.Usint]: 1,
  [IecType.Uint]: 2,
  [IecType.Udint]: 4,
  [IecType.Ulint]: 8,
  [IecType.Real]: 4,
  [IecType.Lreal]: 8,
  [IecType.Time]: 4,
  [IecType.Date]: 2,
  [IecType.Tod]: 4,
  [IecType.Dt]: 6,
  [IecType.String]: 254,
  [IecType.Wstring]: 510
}
