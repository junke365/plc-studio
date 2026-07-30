/** IEC 61131-3 数据类型 */
export enum DataType {
  BOOL = 'BOOL',
  SINT = 'SINT',
  INT = 'INT',
  DINT = 'DINT',
  LINT = 'LINT',
  USINT = 'USINT',
  UINT = 'UINT',
  UDINT = 'UDINT',
  ULINT = 'ULINT',
  BYTE = 'BYTE',
  WORD = 'WORD',
  DWORD = 'DWORD',
  LWORD = 'LWORD',
  REAL = 'REAL',
  LREAL = 'LREAL',
  TIME = 'TIME',
  STRING = 'STRING',
}

/** 变量属性（与运行时 plc_var.h 一致） */
export enum VarAttr {
  LOCAL = 0x00,
  INPUT = 0x01,
  OUTPUT = 0x02,
  INOUT = 0x03,
  GLOBAL = 0x04,
  TEMP = 0x05,
}

/** POU 类型 */
export enum PouType {
  PROGRAM = 'PROGRAM',
  FUNCTION_BLOCK = 'FUNCTION_BLOCK',
  FUNCTION = 'FUNCTION',
}

/** 数据类型大小（字节） */
export function dataTypeSize(dt: DataType): number {
  switch (dt) {
    case DataType.BOOL: return 1;
    case DataType.SINT: case DataType.USINT: case DataType.BYTE: return 1;
    case DataType.INT: case DataType.UINT: case DataType.WORD: return 2;
    case DataType.DINT: case DataType.UDINT: case DataType.DWORD: case DataType.REAL: return 4;
    case DataType.LINT: case DataType.ULINT: case DataType.LWORD: case DataType.LREAL: case DataType.TIME: return 8;
    case DataType.STRING: return 256;
  }
}

/** C 类型名映射 */
export function dataTypeToC(dt: DataType): string {
  const map: Record<string, string> = {
    [DataType.BOOL]: 'plc_bool',
    [DataType.SINT]: 'plc_sint',
    [DataType.INT]: 'plc_int',
    [DataType.DINT]: 'plc_dint',
    [DataType.LINT]: 'plc_lint',
    [DataType.USINT]: 'plc_usint',
    [DataType.UINT]: 'plc_uint',
    [DataType.UDINT]: 'plc_udint',
    [DataType.ULINT]: 'plc_ulint',
    [DataType.BYTE]: 'plc_byte',
    [DataType.WORD]: 'plc_word',
    [DataType.DWORD]: 'plc_dword',
    [DataType.LWORD]: 'plc_lword',
    [DataType.REAL]: 'plc_real',
    [DataType.LREAL]: 'plc_lreal',
    [DataType.TIME]: 'uint64_t',
    [DataType.STRING]: 'char*',
  };
  return map[dt] || 'plc_int';
}

/** 运行时 VarType 枚举名映射 */
export function dataTypeToVarTypeEnum(dt: DataType): string {
  const map: Record<string, string> = {
    [DataType.BOOL]: 'VAR_TYPE_BOOL',
    [DataType.SINT]: 'VAR_TYPE_SINT',
    [DataType.INT]: 'VAR_TYPE_INT',
    [DataType.DINT]: 'VAR_TYPE_DINT',
    [DataType.LINT]: 'VAR_TYPE_LINT',
    [DataType.USINT]: 'VAR_TYPE_USINT',
    [DataType.UINT]: 'VAR_TYPE_UINT',
    [DataType.UDINT]: 'VAR_TYPE_UDINT',
    [DataType.ULINT]: 'VAR_TYPE_ULINT',
    [DataType.BYTE]: 'VAR_TYPE_BYTE',
    [DataType.WORD]: 'VAR_TYPE_WORD',
    [DataType.DWORD]: 'VAR_TYPE_DWORD',
    [DataType.LWORD]: 'VAR_TYPE_LWORD',
    [DataType.REAL]: 'VAR_TYPE_REAL',
    [DataType.LREAL]: 'VAR_TYPE_LREAL',
    [DataType.TIME]: 'VAR_TYPE_TIME',
    [DataType.STRING]: 'VAR_TYPE_STRING',
  };
  return map[dt] || 'VAR_TYPE_INT';
}

export function varAttrToEnum(attr: VarAttr): string {
  const map: Record<number, string> = {
    [VarAttr.LOCAL]: 'VAR_ATTR_LOCAL',
    [VarAttr.INPUT]: 'VAR_ATTR_INPUT',
    [VarAttr.OUTPUT]: 'VAR_ATTR_OUTPUT',
    [VarAttr.INOUT]: 'VAR_ATTR_INOUT',
    [VarAttr.GLOBAL]: 'VAR_ATTR_GLOBAL',
    [VarAttr.TEMP]: 'VAR_ATTR_TEMP',
  };
  return map[attr] || 'VAR_ATTR_LOCAL';
}

/** IEC 61131-3 数字字面量后缀 → DataType */
export function literalSuffixToType(suffix: string): DataType | null {
  switch (suffix.toUpperCase()) {
    case '': return DataType.INT;
    case 'D': return DataType.DINT;
    case 'L': return DataType.LINT;
    case 'UD': return DataType.UDINT;
    case 'UL': return DataType.ULINT;
    case 'R': return DataType.REAL;
    case 'LR': return DataType.LREAL;
    default: return null;
  }
}
