/** IEC 61131-3 ST 词法分析器 */

export enum TokenKind {
  // 关键字
  PROGRAM = 'PROGRAM',
  END_PROGRAM = 'END_PROGRAM',
  FUNCTION = 'FUNCTION',
  END_FUNCTION = 'END_FUNCTION',
  FUNCTION_BLOCK = 'FUNCTION_BLOCK',
  END_FUNCTION_BLOCK = 'END_FUNCTION_BLOCK',
  VAR = 'VAR',
  VAR_INPUT = 'VAR_INPUT',
  VAR_OUTPUT = 'VAR_OUTPUT',
  VAR_IN_OUT = 'VAR_IN_OUT',
  VAR_GLOBAL = 'VAR_GLOBAL',
  VAR_TEMP = 'VAR_TEMP',
  END_VAR = 'END_VAR',
  IF = 'IF',
  THEN = 'THEN',
  ELSIF = 'ELSIF',
  ELSE = 'ELSE',
  END_IF = 'END_IF',
  CASE = 'CASE',
  OF = 'OF',
  END_CASE = 'END_CASE',
  FOR = 'FOR',
  TO = 'TO',
  BY = 'BY',
  DO = 'DO',
  END_FOR = 'END_FOR',
  WHILE = 'WHILE',
  END_WHILE = 'END_WHILE',
  REPEAT = 'REPEAT',
  UNTIL = 'UNTIL',
  END_REPEAT = 'END_REPEAT',
  EXIT = 'EXIT',
  RETURN = 'RETURN',
  TRUE = 'TRUE',
  FALSE = 'FALSE',
  AND = 'AND',
  OR = 'OR',
  XOR = 'XOR',
  NOT = 'NOT',
  MOD = 'MOD',
  // 分隔符
  LPAREN = '(',
  RPAREN = ')',
  LBRACKET = '[',
  RBRACKET = ']',
  DOT = '.',
  COMMA = ',',
  SEMI = ';',
  COLON = ':',
  ASSIGN = ':=',
  // 运算符
  PLUS = '+',
  MINUS = '-',
  STAR = '*',
  SLASH = '/',
  EQ = '=',
  NE = '<>',
  LT = '<',
  GT = '>',
  LE = '<=',
  GE = '>=',
  // 字面量
  INT_LIT = 'INT_LIT',
  REAL_LIT = 'REAL_LIT',
  STRING_LIT = 'STRING_LIT',
  TIME_LIT = 'TIME_LIT',
  // 标识符
  IDENT = 'IDENT',
  // 特殊
  EOF = 'EOF',
  UNKNOWN = 'UNKNOWN',
}

export interface Token {
  kind: TokenKind;
  value: string;
  line: number;
  column: number;
}

const KEYWORDS: Record<string, TokenKind> = {
  'PROGRAM': TokenKind.PROGRAM,
  'END_PROGRAM': TokenKind.END_PROGRAM,
  'FUNCTION': TokenKind.FUNCTION,
  'END_FUNCTION': TokenKind.END_FUNCTION,
  'FUNCTION_BLOCK': TokenKind.FUNCTION_BLOCK,
  'END_FUNCTION_BLOCK': TokenKind.END_FUNCTION_BLOCK,
  'VAR': TokenKind.VAR,
  'VAR_INPUT': TokenKind.VAR_INPUT,
  'VAR_OUTPUT': TokenKind.VAR_OUTPUT,
  'VAR_IN_OUT': TokenKind.VAR_IN_OUT,
  'VAR_GLOBAL': TokenKind.VAR_GLOBAL,
  'VAR_TEMP': TokenKind.VAR_TEMP,
  'END_VAR': TokenKind.END_VAR,
  'IF': TokenKind.IF,
  'THEN': TokenKind.THEN,
  'ELSIF': TokenKind.ELSIF,
  'ELSE': TokenKind.ELSE,
  'END_IF': TokenKind.END_IF,
  'CASE': TokenKind.CASE,
  'OF': TokenKind.OF,
  'END_CASE': TokenKind.END_CASE,
  'FOR': TokenKind.FOR,
  'TO': TokenKind.TO,
  'BY': TokenKind.BY,
  'DO': TokenKind.DO,
  'END_FOR': TokenKind.END_FOR,
  'WHILE': TokenKind.WHILE,
  'END_WHILE': TokenKind.END_WHILE,
  'REPEAT': TokenKind.REPEAT,
  'UNTIL': TokenKind.UNTIL,
  'END_REPEAT': TokenKind.END_REPEAT,
  'EXIT': TokenKind.EXIT,
  'RETURN': TokenKind.RETURN,
  'TRUE': TokenKind.TRUE,
  'FALSE': TokenKind.FALSE,
  'AND': TokenKind.AND,
  'OR': TokenKind.OR,
  'XOR': TokenKind.XOR,
  'NOT': TokenKind.NOT,
  'MOD': TokenKind.MOD,
};

// 两字符运算符映射
const TWO_CHAR_OPS: Record<string, TokenKind> = {
  ':=': TokenKind.ASSIGN,
  '<>': TokenKind.NE,
  '<=': TokenKind.LE,
  '>=': TokenKind.GE,
};

export class StLexer {
  private input: string;
  private pos = 0;
  private line = 1;
  private column = 1;
  private tokens: Token[] = [];
  private tokenized = false;

  constructor(input: string) {
    this.input = input;
  }

  /** 一次性获取所有 Token */
  tokenize(): Token[] {
    if (this.tokenized) return this.tokens;
    this.tokenized = true;

    while (this.pos < this.input.length) {
      const ch = this.peek();

      // 空白
      if (ch === ' ' || ch === '\t' || ch === '\r') {
        this.advance();
        continue;
      }
      if (ch === '\n') {
        this.advance();
        this.line++;
        this.column = 1;
        continue;
      }

      // 注释 (* ... *)
      if (ch === '(' && this.peek(1) === '*') {
        this.skipBlockComment();
        continue;
      }
      // 注释 //
      if (ch === '/' && this.peek(1) === '/') {
        this.skipLineComment();
        continue;
      }

      // 两字符运算符
      const twoChar = ch + this.peek(1);
      if (TWO_CHAR_OPS[twoChar]) {
        this.addToken(TWO_CHAR_OPS[twoChar], twoChar);
        this.advance();
        this.advance();
        continue;
      }

      // 单字符分隔符/运算符
      const singleMap: Record<string, TokenKind> = {
        '(': TokenKind.LPAREN, ')': TokenKind.RPAREN,
        '[': TokenKind.LBRACKET, ']': TokenKind.RBRACKET,
        '.': TokenKind.DOT, ',': TokenKind.COMMA,
        ';': TokenKind.SEMI, ':': TokenKind.COLON,
        '+': TokenKind.PLUS, '-': TokenKind.MINUS,
        '*': TokenKind.STAR, '/': TokenKind.SLASH,
        '=': TokenKind.EQ, '<': TokenKind.LT, '>': TokenKind.GT,
      };
      if (singleMap[ch]) {
        this.addToken(singleMap[ch], ch);
        this.advance();
        continue;
      }

      // 字符串
      if (ch === "'") {
        this.readString();
        continue;
      }

      // 数字
      if (this.isDigit(ch) || (ch === '#')) {
        this.readNumber();
        continue;
      }

      // 标识符或关键字
      if (this.isIdentStart(ch)) {
        this.readIdent();
        continue;
      }

      // 未知字符
      this.addToken(TokenKind.UNKNOWN, ch);
      this.advance();
    }

    this.addToken(TokenKind.EOF, '');
    return this.tokens;
  }

  private peek(offset = 0): string {
    const idx = this.pos + offset;
    return idx < this.input.length ? this.input[idx] : '';
  }

  private advance(): void {
    this.pos++;
    this.column++;
  }

  private addToken(kind: TokenKind, value: string): void {
    this.tokens.push({ kind, value, line: this.line, column: this.column });
  }

  private isDigit(ch: string): boolean {
    return ch >= '0' && ch <= '9';
  }

  private isIdentStart(ch: string): boolean {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch === '_';
  }

  private isIdentPart(ch: string): boolean {
    return this.isIdentStart(ch) || this.isDigit(ch);
  }

  /** 读取标识符或关键字 */
  private readIdent(): void {
    const start = this.pos;
    while (this.isIdentPart(this.peek())) this.advance();
    const word = this.input.slice(start, this.pos);
    const upper = word.toUpperCase();
    const kind = KEYWORDS[upper] || TokenKind.IDENT;
    this.addToken(kind, word);
  }

  /** 读取数字字面量（含类型后缀） */
  private readNumber(): void {
    const start = this.pos;

    // 检查是否为时间字面量（T# / TIME#）
    if (this.peek() === '#') {
      this.advance();
      while (this.isIdentPart(this.peek()) || this.peek() === '_' ||
             this.peek() === '#' || this.peek() === '-') this.advance();
      this.addToken(TokenKind.TIME_LIT, this.input.slice(start, this.pos));
      return;
    }

    // 整数或实数
    let isReal = false;
    while (this.isDigit(this.peek()) || this.peek() === '_') this.advance();
    if (this.peek() === '.') {
      isReal = true;
      this.advance(); // '.'
      while (this.isDigit(this.peek()) || this.peek() === '_') this.advance();
    }
    // 指数
    if (this.peek() === 'E' || this.peek() === 'e') {
      isReal = true;
      this.advance();
      if (this.peek() === '+' || this.peek() === '-') this.advance();
      while (this.isDigit(this.peek())) this.advance();
    }
    // 类型后缀（跳过）
    if (this.isIdentStart(this.peek())) {
      while (this.isIdentPart(this.peek())) this.advance();
    }
    const kind = isReal ? TokenKind.REAL_LIT : TokenKind.INT_LIT;
    this.addToken(kind, this.input.slice(start, this.pos));
  }

  /** 读取字符串（单引号包裹） */
  private readString(): void {
    const start = this.pos;
    this.advance(); // 开头的 '
    while (this.pos < this.input.length && this.peek() !== "'") {
      if (this.peek() === '\n') { this.line++; this.column = 1; }
      else this.column++;
      this.advance();
    }
    if (this.peek() === "'") this.advance(); // 结尾的 '
    this.addToken(TokenKind.STRING_LIT, this.input.slice(start, this.pos));
  }

  /** 跳过块注释 (* ... *) */
  private skipBlockComment(): void {
    this.advance(); this.advance(); // (*
    while (this.pos < this.input.length) {
      if (this.peek() === '*' && this.peek(1) === ')') {
        this.advance(); this.advance(); // *)
        return;
      }
      if (this.peek() === '\n') { this.line++; this.column = 1; }
      else this.column++;
      this.advance();
    }
  }

  /** 跳过行注释 // */
  private skipLineComment(): void {
    while (this.pos < this.input.length && this.peek() !== '\n') {
      this.advance();
    }
  }
}
