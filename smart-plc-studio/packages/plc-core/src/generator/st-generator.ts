import type { PouUnit } from '../model/program.js'
import type { Expr, Stmt } from '../model/expression.js'
import { StmtKind } from '../model/expression.js'
import { DataType, dataTypeToC, dataTypeToVarTypeEnum, varAttrToEnum } from '../model/types.js'
import { collectAllVars } from '../model/variable.js'

export interface GeneratedCode {
  init: string
  main: string
  header: string
}

export class StGenerator {
  private indent = 0
  private pouName = ''

  generate(units: PouUnit[]): GeneratedCode {
    const header = this.generateHeader(units)
    const init = this.generateInit(units)
    const main = this.generateMain(units)
    return { header, init, main }
  }

  private p(s: string): string {
    return '  '.repeat(this.indent) + s + '\n'
  }

  private generateHeader(units: PouUnit[]): string {
    let code = `#ifndef PLC_GENERATED_H\n#define PLC_GENERATED_H\n\n`
    code += `#include "plc_runtime.h"\n\n`

    for (const unit of units) {
      const allVars = collectAllVars(unit.vars)
      if (allVars.length === 0) continue
      code += `/* ${unit.name} 变量 */\n`
      for (const v of allVars) {
        const cType = dataTypeToC(v.type)
        code += `extern ${cType} ${unit.name}_${v.name};\n`
      }
      code += '\n'
    }

    code += `/* 生成代码接口 */\n`
    code += `void generated_init(PlcVarTable* var_table, PlcIoConfig* io_config);\n`
    code += `void generated_main(void);\n`
    code += `uint32_t generated_pou_count(void);\n`
    code += `const char* generated_pou_name(uint32_t index);\n\n`
    code += `#endif /* PLC_GENERATED_H */\n`
    return code
  }

  private generateInit(units: PouUnit[]): string {
    let code = `#include "plc_generated.h"\n\n`
    code += `/* ========== POU 全局变量声明 ========== */\n\n`

    // 声明每个 POU 的全局变量
    for (const unit of units) {
      const allVars = collectAllVars(unit.vars)
      if (allVars.length === 0) continue
      code += `/* ${unit.name} 变量 */\n`
      for (const v of allVars) {
        const cType = dataTypeToC(v.type)
        code += `${cType} ${unit.name}_${v.name};\n`
      }
      code += '\n'
    }

    code += `/* ========== generated_init ========== */\n`
    code += `void generated_init(PlcVarTable* var_table, PlcIoConfig* io_config) {\n`
    this.indent++

    // 注册变量
    for (const unit of units) {
      const allVars = collectAllVars(unit.vars)
      for (const v of allVars) {
        const varTypeEnum = dataTypeToVarTypeEnum(v.type)
        const size = `${dataTypeToC(v.type)}`
        code += this.p(`plc_var_register(var_table, "${unit.name}.${v.name}", ${varTypeEnum}, ${varAttrToEnum(v.attr)}, sizeof(${size}), "");`)
      }
    }

    // 初始化变量初始值
    for (const unit of units) {
      const allVars = collectAllVars(unit.vars)
      for (const v of allVars) {
        if (v.initial) {
          code += this.p(`${unit.name}_${v.name} = ${v.initial};`)
        }
      }
    }

    this.indent--
    code += `}\n\n`
    return code
  }

  private generateMain(units: PouUnit[]): string {
    let code = `#include "plc_generated.h"\n\n`

    code += `/* ========== POU 实现 ========== */\n\n`
    for (const unit of units) {
      code += this.generatePouFunction(unit)
    }
    code += '\n'

    code += `/* ========== generated_main ========== */\n`
    code += `void generated_main(void) {\n`
    this.indent++
    for (const unit of units) {
      code += this.p(`${unit.name}_body();`)
    }
    this.indent--
    code += `}\n\n`

    // POU 计数和名称查询
    code += `uint32_t generated_pou_count(void) { return ${units.length}; }\n\n`
    code += `const char* generated_pou_name(uint32_t index) {\n`
    code += `  switch (index) {\n`
    for (let i = 0; i < units.length; i++) {
      code += `    case ${i}: return "${units[i].name}";\n`
    }
    code += `    default: return "";\n`
    code += `  }\n`
    code += `}\n`

    return code
  }

  private generatePouFunction(unit: PouUnit): string {
    this.pouName = unit.name
    let code = `static void ${unit.name}_body(void) {\n`
    this.indent++
    for (const stmt of unit.body.statements) {
      code += this.generateStatement(stmt)
    }
    this.indent--
    code += `}\n\n`
    return code
  }

  private generateStatement(stmt: Stmt): string {
    switch (stmt.kind) {
      case StmtKind.ASSIGNMENT: {
        const lvalue = this.generateLValue(stmt.lvalue)
        const value = this.generateExpr(stmt.value)
        return this.p(`${lvalue} = ${value};`)
      }
      case StmtKind.IF: {
        let code = ''
        for (let i = 0; i < stmt.branches.length; i++) {
          const branch = stmt.branches[i]
          const prefix = i === 0 ? 'if' : 'else if'
          code += this.p(`${prefix} (${this.generateExpr(branch.condition)}) {`)
          this.indent++
          for (const s of branch.body) code += this.generateStatement(s)
          this.indent--
          code += this.p('}')
        }
        if (stmt.elseBody.length > 0) {
          code += this.p('else {')
          this.indent++
          for (const s of stmt.elseBody) code += this.generateStatement(s)
          this.indent--
          code += this.p('}')
        }
        return code
      }
      case StmtKind.CASE: {
        let code = this.p(`switch (${this.generateExpr(stmt.selector)}) {`)
        this.indent++
        for (const branch of stmt.branches) {
          for (const v of branch.values) {
            code += this.p(`case ${this.generateExpr(v)}:`)
          }
          this.indent++
          for (const s of branch.body) code += this.generateStatement(s)
          code += this.p('break;')
          this.indent--
        }
        if (stmt.elseBody.length > 0) {
          code += this.p('default:')
          this.indent++
          for (const s of stmt.elseBody) code += this.generateStatement(s)
          code += this.p('break;')
          this.indent--
        }
        this.indent--
        code += this.p('}')
        return code
      }
      case StmtKind.FOR: {
        const by = stmt.byStep ? `; ${stmt.variable} += ${this.generateExpr(stmt.byStep)}` : '; ++' + stmt.variable
        let code = this.p(`for (${stmt.variable} = ${this.generateExpr(stmt.initial)}; ${stmt.variable} <= ${this.generateExpr(stmt.end)}${by}) {`)
        this.indent++
        for (const s of stmt.body) code += this.generateStatement(s)
        this.indent--
        code += this.p('}')
        return code
      }
      case StmtKind.WHILE: {
        let code = this.p(`while (${this.generateExpr(stmt.condition)}) {`)
        this.indent++
        for (const s of stmt.body) code += this.generateStatement(s)
        this.indent--
        code += this.p('}')
        return code
      }
      case StmtKind.REPEAT: {
        let code = this.p('do {')
        this.indent++
        for (const s of stmt.body) code += this.generateStatement(s)
        this.indent--
        code += this.p(`} while (!(${this.generateExpr(stmt.until)}));`)
        return code
      }
      case StmtKind.FB_CALL: {
        const args = stmt.args.map(a =>
          a.paramName ? `.${a.paramName} = ${this.generateExpr(a.value)}` : this.generateExpr(a.value)
        ).join(', ')
        return this.p(`${stmt.instanceName}(${args});`)
      }
      case StmtKind.EXIT:
        return this.p('break;')
      case StmtKind.RETURN:
        return this.p('return;')
      default:
        return ''
    }
  }

  private generateLValue(lv: { name: string; indices?: Expr[]; members?: string[] }): string {
    let code = `${this.pouName}_${lv.name}`
    if (lv.indices) {
      for (const idx of lv.indices) {
        code += `[${this.generateExpr(idx)}]`
      }
    }
    if (lv.members) {
      for (const m of lv.members) {
        code += `.${m}`
      }
    }
    return code
  }

  private generateExpr(expr: Expr): string {
    switch (expr.kind) {
      case 'literal':
        if (expr.type === DataType.BOOL) {
          return expr.value === 'TRUE' ? '1' : '0'
        }
        return expr.value
      case 'var_ref':
        return `${this.pouName}_${expr.name}`
      case 'binary':
        return `(${this.generateExpr(expr.left)} ${this.binOpToC(expr.op)} ${this.generateExpr(expr.right)})`
      case 'unary':
        if (expr.op === 'NOT') return `(!(${this.generateExpr(expr.operand)}))`
        return `(-(${this.generateExpr(expr.operand)}))`
      case 'fn_call':
        return `${expr.name}(${expr.args.map(a => this.generateExpr(a.value)).join(', ')})`
      case 'fb_call':
        return `${expr.instanceName}(${expr.args.map(a => this.generateExpr(a.value)).join(', ')})`
      case 'array_access':
        return `${this.generateExpr(expr.target)}[${this.generateExpr(expr.index)}]`
      case 'member_access':
        return `${this.generateExpr(expr.target)}.${expr.member}`
      default:
        return '0'
    }
  }

  private binOpToC(op: string): string {
    const map: Record<string, string> = {
      'AND': '&&',
      'OR': '||',
      'XOR': '^',
      '+': '+',
      '-': '-',
      '*': '*',
      '/': '/',
      'MOD': '%',
      '>': '>',
      '>=': '>=',
      '<': '<',
      '<=': '<=',
      '=': '==',
      '<>': '!=',
      '**': '/*pow*/',
    }
    return map[op] || op
  }
}
