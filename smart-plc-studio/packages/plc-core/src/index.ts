export { StLexer, TokenKind } from './parser/st-lexer.js'
export type { Token } from './parser/st-lexer.js'
export { StParser } from './parser/st-parser.js'
export { StValidator } from './validator/st-validator.js'
export type { ValidationResult } from './validator/st-validator.js'
export { StGenerator } from './generator/st-generator.js'
export type { GeneratedCode } from './generator/st-generator.js'

export { DataType, VarAttr, PouType } from './model/types.js'
export { ExprKind, StmtKind, BinOp, UnaryOp } from './model/expression.js'
export type {
  Expr, Stmt, Literal, VarRef, BinaryExpr, UnaryExpr,
  FunctionCall, FBCall, FBCallStmt, ExitStmt, ReturnStmt,
  ArrayAccess, MemberAccess,
  CallArg, IfBranch, CaseBranch, LValue,
} from './model/expression.js'
export type { PouUnit, PouBody, PouReturn, CompileResult, CompileError } from './model/program.js'
export type { VariableDecl, VarBlock, VariableTable } from './model/variable.js'
export { collectAllVars } from './model/variable.js'
export { dataTypeSize, dataTypeToC, dataTypeToVarTypeEnum, varAttrToEnum, literalSuffixToType } from './model/types.js'

import { StParser } from './parser/st-parser.js'
import { StValidator } from './validator/st-validator.js'
import { StGenerator } from './generator/st-generator.js'
import type { CompileResult } from './model/program.js'

/** 一键编译：词法分析 → 语法分析 → 验证 → 代码生成 */
export function compileStCode(source: string): CompileResult {
  const parser = new StParser(source)
  const result = parser.parse()

  if (result.units.length > 0) {
    const validator = new StValidator()
    const vResult = validator.validate(result.units)
    result.errors.push(...vResult.errors)

    const hasErrors = vResult.errors.some(e => e.severity === 'error')
    if (!hasErrors) {
      const generator = new StGenerator()
      const gen = generator.generate(result.units)
      result.generatedInit = gen.init
      result.generatedMain = gen.main
      result.generatedH = gen.header
    }
  }

  return result
}
