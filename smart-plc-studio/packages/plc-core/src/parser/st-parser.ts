import { TokenKind, type Token, StLexer } from './st-lexer.js'
import { DataType, PouType, VarAttr } from '../model/types.js'
import type { PouUnit, PouBody, CompileResult, CompileError } from '../model/program.js'
import type { VariableTable, VariableDecl, VarBlock } from '../model/variable.js'
import {
  StmtKind, BinOp, UnaryOp,
  type Expr, type Stmt, type Literal, type VarRef,
  type FunctionCall, type FBCallStmt,
  type ArrayAccess, type MemberAccess,
  type CallArg, type IfBranch, type CaseBranch,
  type LValue,
} from '../model/expression.js'

export class StParser {
  private tokens: Token[] = []
  private pos = 0
  private errors: CompileError[] = []

  constructor(input: string) {
    const lexer = new StLexer(input)
    this.tokens = lexer.tokenize()
  }

  private get current(): Token {
    return this.tokens[this.pos] || this.tokens[this.tokens.length - 1]
  }

  private peek(kind: TokenKind): boolean {
    return this.current.kind === kind
  }

  private expect(kind: TokenKind, msg: string): Token {
    if (this.current.kind === kind) {
      const t = this.current
      this.pos++
      return t
    }
    this.error(msg)
    return { kind, value: '', line: this.current.line, column: this.current.column }
  }

  private error(msg: string) {
    this.errors.push({
      line: this.current.line,
      column: this.current.column,
      message: msg,
      severity: 'error',
    })
  }

  private skipPast(...kinds: TokenKind[]) {
    while (!this.peek(TokenKind.EOF)) {
      if (kinds.includes(this.current.kind)) return
      this.pos++
    }
  }

  parse(): CompileResult {
    const units: PouUnit[] = []

    while (!this.peek(TokenKind.EOF)) {
      if (this.peek(TokenKind.PROGRAM)) {
        const u = this.parseProgram()
        if (u) units.push(u)
      } else if (this.peek(TokenKind.FUNCTION_BLOCK)) {
        const u = this.parseFunctionBlock()
        if (u) units.push(u)
      } else if (this.peek(TokenKind.FUNCTION)) {
        const u = this.parseFunction()
        if (u) units.push(u)
      } else {
        break
      }
    }

    return {
      units,
      errors: this.errors,
      generatedInit: '',
      generatedMain: '',
      generatedH: '',
    }
  }

  private parseProgram(): PouUnit | null {
    this.expect(TokenKind.PROGRAM, '期望 PROGRAM')
    const nameToken = this.expect(TokenKind.IDENT, '期望程序名')
    const vars = this.parseVarBlocks()
    const body = this.parseBody()
    this.expect(TokenKind.END_PROGRAM, '期望 END_PROGRAM')
    return {
      type: PouType.PROGRAM,
      name: nameToken.value,
      vars,
      body,
    }
  }

  private parseFunctionBlock(): PouUnit | null {
    this.expect(TokenKind.FUNCTION_BLOCK, '期望 FUNCTION_BLOCK')
    const nameToken = this.expect(TokenKind.IDENT, '期望功能块名')
    const vars = this.parseVarBlocks()
    const body = this.parseBody()
    this.expect(TokenKind.END_FUNCTION_BLOCK, '期望 END_FUNCTION_BLOCK')
    return {
      type: PouType.FUNCTION_BLOCK,
      name: nameToken.value,
      vars,
      body,
    }
  }

  private parseFunction(): PouUnit | null {
    this.expect(TokenKind.FUNCTION, '期望 FUNCTION')
    const nameToken = this.expect(TokenKind.IDENT, '期望函数名')
    let returnType: DataType | undefined
    if (this.peek(TokenKind.COLON)) {
      this.pos++
      returnType = this.parseDataType()
    }
    const vars = this.parseVarBlocks()
    const body = this.parseBody()
    this.expect(TokenKind.END_FUNCTION, '期望 END_FUNCTION')
    return {
      type: PouType.FUNCTION,
      name: nameToken.value,
      returnType,
      vars,
      body,
    }
  }

  private parseVarBlocks(): VariableTable {
    const blocks: VarBlock[] = []
    while (this.peek(TokenKind.VAR) || this.peek(TokenKind.VAR_INPUT) ||
           this.peek(TokenKind.VAR_OUTPUT) || this.peek(TokenKind.VAR_IN_OUT) ||
           this.peek(TokenKind.VAR_GLOBAL) || this.peek(TokenKind.VAR_TEMP)) {
      const block = this.parseVarBlock()
      if (block) blocks.push(block)
    }
    return { blocks }
  }

  private parseVarBlock(): VarBlock | null {
    let attr: VarAttr
    if (this.peek(TokenKind.VAR)) { attr = VarAttr.LOCAL; this.pos++ }
    else if (this.peek(TokenKind.VAR_INPUT)) { attr = VarAttr.INPUT; this.pos++ }
    else if (this.peek(TokenKind.VAR_OUTPUT)) { attr = VarAttr.OUTPUT; this.pos++ }
    else if (this.peek(TokenKind.VAR_IN_OUT)) { attr = VarAttr.INOUT; this.pos++ }
    else if (this.peek(TokenKind.VAR_GLOBAL)) { attr = VarAttr.GLOBAL; this.pos++ }
    else if (this.peek(TokenKind.VAR_TEMP)) { attr = VarAttr.TEMP; this.pos++ }
    else return null

    const vars: VariableDecl[] = []
    while (!this.peek(TokenKind.END_VAR) && !this.peek(TokenKind.EOF)) {
      // 允许空 VAR 块
      if (this.peek(TokenKind.SEMI)) { this.pos++; continue }
      const decls = this.parseVarDecls(attr)
      for (const d of decls) vars.push(d)
    }
    this.expect(TokenKind.END_VAR, '期望 END_VAR')
    return { attr, vars }
  }

  private parseVarDecls(attr: VarAttr): VariableDecl[] {
    const names: string[] = []
    names.push(this.expect(TokenKind.IDENT, '期望变量名').value)
    while (this.peek(TokenKind.COMMA)) {
      this.pos++
      names.push(this.expect(TokenKind.IDENT, '期望变量名').value)
    }
    this.expect(TokenKind.COLON, '期望冒号')
    const type = this.parseDataType()
    let initial: string | undefined
    if (this.peek(TokenKind.ASSIGN)) {
      this.pos++
      initial = this.readInitialValue()
    }
    this.expect(TokenKind.SEMI, '期望分号')
    return names.map(name => ({ name, type, attr, comment: '', initial }))
  }

  private parseDataType(): DataType {
    const t = this.current
    this.pos++
    const upper = t.value.toUpperCase()
    switch (upper) {
      case 'BOOL': return DataType.BOOL
      case 'SINT': return DataType.SINT
      case 'INT': return DataType.INT
      case 'DINT': return DataType.DINT
      case 'LINT': return DataType.LINT
      case 'USINT': return DataType.USINT
      case 'UINT': return DataType.UINT
      case 'UDINT': return DataType.UDINT
      case 'ULINT': return DataType.ULINT
      case 'BYTE': return DataType.BYTE
      case 'WORD': return DataType.WORD
      case 'DWORD': return DataType.DWORD
      case 'LWORD': return DataType.LWORD
      case 'REAL': return DataType.REAL
      case 'LREAL': return DataType.LREAL
      case 'TIME': return DataType.TIME
      case 'STRING': return DataType.STRING
      default:
        this.error(`未知数据类型: ${t.value}`)
        return DataType.INT
    }
  }

  private readInitialValue(): string {
    const start = this.pos
    // 读取直到分号或行尾
    while (!this.peek(TokenKind.SEMI) && !this.peek(TokenKind.EOF)) {
      this.pos++
    }
    return this.tokens.slice(start, this.pos).map(t => t.value).join(' ').trim()
  }

  private parseBody(): PouBody {
    const statements: Stmt[] = []
    while (!this.isBlockEnd()) {
      // 跳过空语句（单独分号，或 END_IF 后的多余分号）
      if (this.peek(TokenKind.SEMI)) { this.pos++; continue }
      const stmt = this.parseStatement()
      if (stmt) statements.push(stmt)
      else {
        // 跳过无法识别的 token
        if (this.current.kind !== TokenKind.EOF && !this.isBlockEnd()) {
          this.error(`意外的 token: ${this.current.value}`)
          this.pos++
        }
      }
    }
    return { statements }
  }

  private isBlockEnd(): boolean {
    const k = this.current.kind
    return k === TokenKind.END_PROGRAM || k === TokenKind.END_FUNCTION ||
           k === TokenKind.END_FUNCTION_BLOCK || k === TokenKind.END_IF ||
           k === TokenKind.END_CASE || k === TokenKind.END_FOR ||
           k === TokenKind.END_WHILE || k === TokenKind.END_REPEAT ||
           k === TokenKind.ELSE || k === TokenKind.ELSIF ||
           k === TokenKind.UNTIL || k === TokenKind.EOF
  }

  private parseStatement(): Stmt | null {
    // EXIT 语句
    if (this.peek(TokenKind.EXIT)) {
      this.pos++
      this.expect(TokenKind.SEMI, '期望 ;')
      return { kind: StmtKind.EXIT }
    }
    // RETURN 语句
    if (this.peek(TokenKind.RETURN)) {
      this.pos++
      this.expect(TokenKind.SEMI, '期望 ;')
      return { kind: StmtKind.RETURN }
    }
    // IF 语句
    if (this.peek(TokenKind.IF)) return this.parseIfStmt()
    // CASE 语句
    if (this.peek(TokenKind.CASE)) return this.parseCaseStmt()
    // FOR 语句
    if (this.peek(TokenKind.FOR)) return this.parseForStmt()
    // WHILE 语句
    if (this.peek(TokenKind.WHILE)) return this.parseWhileStmt()
    // REPEAT 语句
    if (this.peek(TokenKind.REPEAT)) return this.parseRepeatStmt()

    // 赋值、FB 调用或表达式语句
    const idToken = this.current
    if (idToken.kind === TokenKind.IDENT) {
      this.pos++
      // FB 调用: instanceName(...)
      if (this.peek(TokenKind.LPAREN)) {
        return this.parseFBCallStmt(idToken.value)
      }
      // 赋值: lvalue := expr
      if (this.peek(TokenKind.ASSIGN)) {
        return this.parseAssignmentStmt(idToken.value)
      }
      // 否则是表达式语句（如纯函数调用）
      // 回退处理为函数调用
      this.error(`语法错误: 意外的 '${idToken.value}'`)
      this.skipPast(TokenKind.SEMI)
      return null
    }

    this.error(`期望语句，得到 '${this.current.value}'`)
    this.pos++
    return null
  }

  private parseIfStmt(): Stmt {
    this.expect(TokenKind.IF, '期望 IF')
    const branches: IfBranch[] = []
    const firstCond = this.parseExpression()
    this.expect(TokenKind.THEN, '期望 THEN')
    const firstBody = this.parseBody()
    branches.push({ condition: firstCond, body: firstBody.statements })

    while (this.peek(TokenKind.ELSIF)) {
      this.pos++
      const cond = this.parseExpression()
      this.expect(TokenKind.THEN, '期望 THEN')
      const body = this.parseBody()
      branches.push({ condition: cond, body: body.statements })
    }

    let elseBody: Stmt[] = []
    if (this.peek(TokenKind.ELSE)) {
      this.pos++
      elseBody = this.parseBody().statements
    }
    this.expect(TokenKind.END_IF, '期望 END_IF')
    return { kind: StmtKind.IF, branches, elseBody }
  }

  private parseCaseStmt(): Stmt {
    this.expect(TokenKind.CASE, '期望 CASE')
    const selector = this.parseExpression()
    this.expect(TokenKind.OF, '期望 OF')
    const branches: CaseBranch[] = []
    let elseBody: Stmt[] = []
    while (!this.peek(TokenKind.END_CASE) && !this.peek(TokenKind.EOF)) {
      if (this.peek(TokenKind.ELSE)) {
        this.pos++
        elseBody = this.parseBody().statements
        break
      }
      const values: Expr[] = [this.parseExpression()]
      while (this.peek(TokenKind.COMMA)) {
        this.pos++
        values.push(this.parseExpression())
      }
      this.expect(TokenKind.COLON, '期望 :')
      const body = this.parseBody()
      branches.push({ values, body: body.statements })
    }
    this.expect(TokenKind.END_CASE, '期望 END_CASE')
    return { kind: StmtKind.CASE, selector, branches, elseBody }
  }

  private parseForStmt(): Stmt {
    this.expect(TokenKind.FOR, '期望 FOR')
    const variable = this.expect(TokenKind.IDENT, '期望循环变量').value
    this.expect(TokenKind.ASSIGN, '期望 :=')
    const initial = this.parseExpression()
    this.expect(TokenKind.TO, '期望 TO')
    const end = this.parseExpression()
    let byStep: Expr | undefined
    if (this.peek(TokenKind.BY)) {
      this.pos++
      byStep = this.parseExpression()
    }
    this.expect(TokenKind.DO, '期望 DO')
    const body = this.parseBody()
    this.expect(TokenKind.END_FOR, '期望 END_FOR')
    return { kind: StmtKind.FOR, variable, initial, end, byStep, body: body.statements }
  }

  private parseWhileStmt(): Stmt {
    this.expect(TokenKind.WHILE, '期望 WHILE')
    const condition = this.parseExpression()
    this.expect(TokenKind.DO, '期望 DO')
    const body = this.parseBody()
    this.expect(TokenKind.END_WHILE, '期望 END_WHILE')
    return { kind: StmtKind.WHILE, condition, body: body.statements }
  }

  private parseRepeatStmt(): Stmt {
    this.expect(TokenKind.REPEAT, '期望 REPEAT')
    const body = this.parseBody()
    this.expect(TokenKind.UNTIL, '期望 UNTIL')
    const until = this.parseExpression()
    this.expect(TokenKind.END_REPEAT, '期望 END_REPEAT')
    return { kind: StmtKind.REPEAT, body: body.statements, until }
  }

  private parseFBCallStmt(instanceName: string): FBCallStmt {
    this.expect(TokenKind.LPAREN, '期望 (')
    const args: CallArg[] = this.parseCallArgs()
    this.expect(TokenKind.RPAREN, '期望 )')
    this.expect(TokenKind.SEMI, '期望 ;')
    return { kind: StmtKind.FB_CALL, instanceName, fbType: '', args }
  }

  private parseAssignmentStmt(firstIdent: string): Stmt {
    const lvalue: LValue = { name: firstIdent, indices: [], members: [] }
    // 解析下标和成员访问
    while (this.peek(TokenKind.LBRACKET) || this.peek(TokenKind.DOT)) {
      if (this.peek(TokenKind.LBRACKET)) {
        this.pos++
        const idx = this.parseExpression()
        this.expect(TokenKind.RBRACKET, '期望 ]')
        lvalue.indices!.push(idx)
      } else if (this.peek(TokenKind.DOT)) {
        this.pos++
        lvalue.members!.push(this.expect(TokenKind.IDENT, '期望成员名').value)
      }
    }
    this.expect(TokenKind.ASSIGN, '期望 :=')
    const value = this.parseExpression()
    this.expect(TokenKind.SEMI, '期望 ;')
    return { kind: StmtKind.ASSIGNMENT, lvalue, value }
  }

  // ========== 表达式解析 ==========

  private parseExpression(): Expr {
    return this.parseOrExpr()
  }

  private parseOrExpr(): Expr {
    let left = this.parseXorExpr()
    while (this.peek(TokenKind.OR)) {
      this.pos++
      const right = this.parseXorExpr()
      left = { kind: 'binary', op: BinOp.OR, left, right }
    }
    return left
  }

  private parseXorExpr(): Expr {
    let left = this.parseAndExpr()
    while (this.peek(TokenKind.XOR)) {
      this.pos++
      const right = this.parseAndExpr()
      left = { kind: 'binary', op: BinOp.XOR, left, right }
    }
    return left
  }

  private parseAndExpr(): Expr {
    let left = this.parseComparisonExpr()
    while (this.peek(TokenKind.AND)) {
      this.pos++
      const right = this.parseComparisonExpr()
      left = { kind: 'binary', op: BinOp.AND, left, right }
    }
    return left
  }

  private parseComparisonExpr(): Expr {
    let left = this.parseAddExpr()
    while (
      this.peek(TokenKind.EQ) || this.peek(TokenKind.NE) ||
      this.peek(TokenKind.LT) || this.peek(TokenKind.GT) ||
      this.peek(TokenKind.LE) || this.peek(TokenKind.GE)
    ) {
      const opToken = this.current
      this.pos++
      const opMap: Record<string, BinOp> = {
        '=': BinOp.EQ, '<>': BinOp.NE, '<': BinOp.LT,
        '>': BinOp.GT, '<=': BinOp.LE, '>=': BinOp.GE,
      }
      const right = this.parseAddExpr()
      left = { kind: 'binary', op: opMap[opToken.value] || BinOp.EQ, left, right }
    }
    return left
  }

  private parseAddExpr(): Expr {
    let left = this.parseMulExpr()
    while (this.peek(TokenKind.PLUS) || this.peek(TokenKind.MINUS)) {
      const op = this.current.kind === TokenKind.PLUS ? BinOp.PLUS : BinOp.MINUS
      this.pos++
      const right = this.parseMulExpr()
      left = { kind: 'binary', op, left, right }
    }
    return left
  }

  private parseMulExpr(): Expr {
    let left = this.parseUnaryExpr()
    while (this.peek(TokenKind.STAR) || this.peek(TokenKind.SLASH) || this.peek(TokenKind.MOD)) {
      const op = this.current.kind === TokenKind.STAR ? BinOp.MUL
        : this.current.kind === TokenKind.SLASH ? BinOp.DIV
        : BinOp.MOD
      this.pos++
      const right = this.parseUnaryExpr()
      left = { kind: 'binary', op, left, right }
    }
    return left
  }

  private parseUnaryExpr(): Expr {
    if (this.peek(TokenKind.MINUS)) {
      this.pos++
      const operand = this.parsePrimaryExpr()
      return { kind: 'unary', op: UnaryOp.NEG, operand }
    }
    if (this.peek(TokenKind.NOT)) {
      this.pos++
      const operand = this.parsePrimaryExpr()
      return { kind: 'unary', op: UnaryOp.NOT, operand }
    }
    return this.parsePrimaryExpr()
  }

  private parsePrimaryExpr(): Expr {
    // 字面量
    if (this.peek(TokenKind.INT_LIT)) {
      const t = this.current; this.pos++
      return { kind: 'literal', value: t.value, type: DataType.INT } as Literal
    }
    if (this.peek(TokenKind.REAL_LIT)) {
      const t = this.current; this.pos++
      return { kind: 'literal', value: t.value, type: DataType.REAL } as Literal
    }
    if (this.peek(TokenKind.STRING_LIT)) {
      const t = this.current; this.pos++
      return { kind: 'literal', value: t.value, type: DataType.STRING } as Literal
    }
    if (this.peek(TokenKind.TIME_LIT)) {
      const t = this.current; this.pos++
      return { kind: 'literal', value: t.value, type: DataType.TIME } as Literal
    }
    if (this.peek(TokenKind.TRUE)) {
      this.pos++
      return { kind: 'literal', value: 'TRUE', type: DataType.BOOL } as Literal
    }
    if (this.peek(TokenKind.FALSE)) {
      this.pos++
      return { kind: 'literal', value: 'FALSE', type: DataType.BOOL } as Literal
    }

    // 括号表达式
    if (this.peek(TokenKind.LPAREN)) {
      this.pos++
      const expr = this.parseExpression()
      this.expect(TokenKind.RPAREN, '期望 )')
      return expr
    }

    // 标识符
    if (this.peek(TokenKind.IDENT)) {
      const nameToken = this.current
      this.pos++
      return this.parseIdentifierSuffix(nameToken.value)
    }

    this.error(`期望表达式，得到 '${this.current.value}'`)
    return { kind: 'literal', value: '0', type: DataType.INT } as Literal
  }

  private parseIdentifierSuffix(name: string): Expr {
    // 函数调用: ident(...)
    if (this.peek(TokenKind.LPAREN)) {
      this.pos++
      const args = this.parseCallArgs()
      this.expect(TokenKind.RPAREN, '期望 )')
      return { kind: 'fn_call', name, args } as FunctionCall
    }
    // 数组访问: ident[...]
    if (this.peek(TokenKind.LBRACKET)) {
      this.pos++
      const index = this.parseExpression()
      this.expect(TokenKind.RBRACKET, '期望 ]')
      return { kind: 'array_access', target: { kind: 'var_ref', name } as VarRef, index } as ArrayAccess
    }
    // 成员访问: ident.member
    if (this.peek(TokenKind.DOT)) {
      this.pos++
      const member = this.expect(TokenKind.IDENT, '期望成员名').value
      return { kind: 'member_access', target: { kind: 'var_ref', name } as VarRef, member } as MemberAccess
    }
    // 简单变量引用
    return { kind: 'var_ref', name } as VarRef
  }

  private parseCallArgs(): CallArg[] {
    const args: CallArg[] = []
    while (!this.peek(TokenKind.RPAREN) && !this.peek(TokenKind.EOF)) {
      // 命名参数: param := expr
      if (this.peek(TokenKind.IDENT) && this.tokens[this.pos + 1]?.kind === TokenKind.ASSIGN) {
        const paramName = this.current.value
        this.pos++ // IDENT
        this.pos++ // :=
        const value = this.parseExpression()
        args.push({ paramName, value })
      } else {
        const value = this.parseExpression()
        args.push({ value })
      }
      if (this.peek(TokenKind.COMMA)) this.pos++
    }
    return args
  }
}
