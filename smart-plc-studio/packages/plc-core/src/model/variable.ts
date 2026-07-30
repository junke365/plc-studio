import { DataType, VarAttr } from './types.js';

/** 变量声明 */
export interface VariableDecl {
  name: string;
  type: DataType;
  attr: VarAttr;
  comment: string;
  initial?: string;
}

/** VAR 块声明 */
export interface VarBlock {
  attr: VarAttr;
  vars: VariableDecl[];
}

/** 变量表（一个 POU 的所有变量） */
export interface VariableTable {
  blocks: VarBlock[];
}

export function collectAllVars(table: VariableTable): VariableDecl[] {
  const result: VariableDecl[] = [];
  for (const block of table.blocks) {
    for (const v of block.vars) {
      result.push(v);
    }
  }
  return result;
}
