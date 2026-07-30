import type { PouUnit, CompileError } from '../model/program.js'
import type { Expr, Stmt } from '../model/expression.js'
import { StmtKind } from '../model/expression.js'
import { collectAllVars } from '../model/variable.js'

export interface ValidationResult {
  errors: CompileError[]
}

export class StValidator {
  private errors: CompileError[] = []

  validate(units: PouUnit[]): ValidationResult {
    for (const unit of units) {
      this.validateUnit(unit)
    }
    return { errors: this.errors }
  }

  private validateUnit(unit: PouUnit) {
    const varNames = new Set<string>()
    const allVars = collectAllVars(unit.vars)

    for (const v of allVars) {
      if (varNames.has(v.name)) {
        this.errors.push({
          line: 0, column: 0,
          message: `重复声明变量 '${v.name}' 在 POU '${unit.name}' 中`,
          severity: 'error',
        })
      }
      varNames.add(v.name)
    }

    this.validateStatements(unit.body.statements, varNames, unit.name)
  }

  private validateStatements(stmts: Stmt[], varNames: Set<string>, pouName: string) {
    for (const stmt of stmts) {
      this.validateStatement(stmt, varNames, pouName)
    }
  }

  private validateStatement(stmt: Stmt, varNames: Set<string>, pouName: string) {
    switch (stmt.kind) {
      case StmtKind.ASSIGNMENT: {
        if (!varNames.has(stmt.lvalue.name)) {
          this.errors.push({
            line: 0, column: 0,
            message: `未声明的变量 '${stmt.lvalue.name}' 在 POU '${pouName}' 中`,
            severity: 'error',
          })
        }
        this.validateExpr(stmt.value, varNames)
        break
      }
      case StmtKind.IF:
        for (const branch of stmt.branches) {
          this.validateExpr(branch.condition, varNames)
          this.validateStatements(branch.body, varNames, pouName)
        }
        this.validateStatements(stmt.elseBody, varNames, pouName)
        break
      case StmtKind.CASE:
        this.validateExpr(stmt.selector, varNames)
        for (const branch of stmt.branches) {
          for (const v of branch.values) this.validateExpr(v, varNames)
          this.validateStatements(branch.body, varNames, pouName)
        }
        this.validateStatements(stmt.elseBody, varNames, pouName)
        break
      case StmtKind.FOR:
        if (!varNames.has(stmt.variable)) {
          this.errors.push({
            line: 0, column: 0,
            message: `未声明的循环变量 '${stmt.variable}' 在 POU '${pouName}' 中`,
            severity: 'error',
          })
        }
        this.validateExpr(stmt.initial, varNames)
        this.validateExpr(stmt.end, varNames)
        if (stmt.byStep) this.validateExpr(stmt.byStep, varNames)
        this.validateStatements(stmt.body, varNames, pouName)
        break
      case StmtKind.WHILE:
        this.validateExpr(stmt.condition, varNames)
        this.validateStatements(stmt.body, varNames, pouName)
        break
      case StmtKind.REPEAT:
        this.validateStatements(stmt.body, varNames, pouName)
        this.validateExpr(stmt.until, varNames)
        break
      case StmtKind.FB_CALL:
        for (const arg of stmt.args) {
          this.validateExpr(arg.value, varNames)
        }
        break
    }
  }

  private validateExpr(expr: Expr, varNames: Set<string>) {
    switch (expr.kind) {
      case 'var_ref':
        if (!varNames.has(expr.name)) {
          this.errors.push({
            line: 0, column: 0,
            message: `未声明的变量 '${expr.name}'`,
            severity: 'error',
          })
        }
        break
      case 'binary':
        this.validateExpr(expr.left, varNames)
        this.validateExpr(expr.right, varNames)
        break
      case 'unary':
        this.validateExpr(expr.operand, varNames)
        break
      case 'fn_call':
        for (const arg of expr.args) {
          this.validateExpr(arg.value, varNames)
        }
        break
      case 'fb_call':
        for (const arg of expr.args) {
          this.validateExpr(arg.value, varNames)
        }
        break
      case 'array_access':
        this.validateExpr(expr.target, varNames)
        this.validateExpr(expr.index, varNames)
        break
      case 'member_access':
        this.validateExpr(expr.target, varNames)
        break
    }
  }
}
