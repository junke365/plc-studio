import { DataType } from './types.js';

/** 表达式节点类型 */
export enum ExprKind {
  LITERAL = 'LITERAL',
  VARIABLE = 'VARIABLE',
  BINARY = 'BINARY',
  UNARY = 'UNARY',
  FUNCTION_CALL = 'FUNCTION_CALL',
  FB_CALL = 'FB_CALL',
  ARRAY_ACCESS = 'ARRAY_ACCESS',
  MEMBER_ACCESS = 'MEMBER_ACCESS',
}

/** 二元运算符 */
export enum BinOp {
  AND = 'AND',
  OR = 'OR',
  XOR = 'XOR',
  PLUS = '+',
  MINUS = '-',
  MUL = '*',
  DIV = '/',
  MOD = 'MOD',
  GT = '>',
  GE = '>=',
  LT = '<',
  LE = '<=',
  EQ = '=',
  NE = '<>',
  POWER = '**',
}

/** 一元运算符 */
export enum UnaryOp {
  NOT = 'NOT',
  NEG = '-',
}

/** 字面量 */
export interface Literal {
  kind: 'literal';
  value: string;
  type: DataType;
}

/** 变量引用 */
export interface VarRef {
  kind: 'var_ref';
  name: string;
}

/** 二元表达式 */
export interface BinaryExpr {
  kind: 'binary';
  op: BinOp;
  left: Expr;
  right: Expr;
}

/** 一元表达式 */
export interface UnaryExpr {
  kind: 'unary';
  op: UnaryOp;
  operand: Expr;
}

/** 函数/功能块调用参数 */
export interface CallArg {
  paramName?: string;
  value: Expr;
}

/** 函数调用 */
export interface FunctionCall {
  kind: 'fn_call';
  name: string;
  args: CallArg[];
}

/** 功能块调用 */
export interface FBCall {
  kind: 'fb_call';
  instanceName: string;
  fbType: string;
  args: CallArg[];
}

/** 数组访问 */
export interface ArrayAccess {
  kind: 'array_access';
  target: Expr;
  index: Expr;
}

/** 成员访问 */
export interface MemberAccess {
  kind: 'member_access';
  target: Expr;
  member: string;
}

export type Expr =
  | Literal
  | VarRef
  | BinaryExpr
  | UnaryExpr
  | FunctionCall
  | FBCall
  | ArrayAccess
  | MemberAccess;

/** 语句类型 */
export enum StmtKind {
  ASSIGNMENT = 'ASSIGNMENT',
  IF = 'IF',
  CASE = 'CASE',
  FOR = 'FOR',
  WHILE = 'WHILE',
  REPEAT = 'REPEAT',
  FB_CALL = 'FB_CALL',
  EXIT = 'EXIT',
  RETURN = 'RETURN',
}

/** 左值 */
export interface LValue {
  name: string;
  indices?: Expr[];
  members?: string[];
}

/** 赋值语句 */
export interface AssignStmt {
  kind: StmtKind.ASSIGNMENT;
  lvalue: LValue;
  value: Expr;
}

/** IF 分支 */
export interface IfBranch {
  condition: Expr;
  body: Stmt[];
}

/** IF 语句 */
export interface IfStmt {
  kind: StmtKind.IF;
  branches: IfBranch[];
  elseBody: Stmt[];
}

/** CASE 分支 */
export interface CaseBranch {
  values: Expr[];
  body: Stmt[];
}

/** CASE 语句 */
export interface CaseStmt {
  kind: StmtKind.CASE;
  selector: Expr;
  branches: CaseBranch[];
  elseBody: Stmt[];
}

/** FOR 循环 */
export interface ForStmt {
  kind: StmtKind.FOR;
  variable: string;
  initial: Expr;
  end: Expr;
  byStep?: Expr;
  body: Stmt[];
}

/** WHILE 循环 */
export interface WhileStmt {
  kind: StmtKind.WHILE;
  condition: Expr;
  body: Stmt[];
}

/** REPEAT 循环 */
export interface RepeatStmt {
  kind: StmtKind.REPEAT;
  body: Stmt[];
  until: Expr;
}

/** 功能块调用语句 */
export interface FBCallStmt {
  kind: StmtKind.FB_CALL;
  instanceName: string;
  fbType: string;
  args: CallArg[];
}

/** EXIT 语句 */
export interface ExitStmt {
  kind: StmtKind.EXIT;
}

/** RETURN 语句 */
export interface ReturnStmt {
  kind: StmtKind.RETURN;
}

export type Stmt =
  | AssignStmt
  | IfStmt
  | CaseStmt
  | ForStmt
  | WhileStmt
  | RepeatStmt
  | FBCallStmt
  | ExitStmt
  | ReturnStmt;

/** 用于 ST 语法树的辅助函数 */
export function exprToString(expr: Expr): string {
  switch (expr.kind) {
    case 'literal': return `${expr.value}`;
    case 'var_ref': return expr.name;
    case 'binary': return `(${exprToString(expr.left)} ${expr.op} ${exprToString(expr.right)})`;
    case 'unary': return `${expr.op}(${exprToString(expr.operand)})`;
    case 'fn_call': return `${expr.name}(${expr.args.map(a => a.value ? exprToString(a.value) : '').join(', ')})`;
    case 'fb_call': return `${expr.instanceName}(${expr.args.map(a => exprToString(a.value)).join(', ')})`;
    default: return '?';
  }
}
