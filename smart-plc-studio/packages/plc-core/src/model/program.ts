import { DataType, PouType } from './types.js';
import type { VariableTable } from './variable.js';
import type { Stmt } from './expression.js';

/** POU 输出类型（仅 FUNCTION 有） */
export interface PouReturn {
  type: DataType;
  name: string;  // 通常与 POU 名相同
}

/** POU 体（语句列表） */
export interface PouBody {
  statements: Stmt[];
}

/** 程序组织单元 */
export interface PouUnit {
  type: PouType;
  name: string;
  returnType?: DataType;
  vars: VariableTable;
  body: PouBody;
}

/** 编译输出 */
export interface CompileResult {
  /** 生成的所有 POU */
  units: PouUnit[];
  /** 错误列表 */
  errors: CompileError[];
  /** 生成的主 C 代码 */
  generatedInit: string;
  generatedMain: string;
  generatedH: string;
}

export interface CompileError {
  line: number;
  column: number;
  message: string;
  severity: 'error' | 'warning';
}
