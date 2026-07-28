// IEC 61131-3 ST 自动补全提供者

// ST 关键字补全项
const stKeywords = [
  {
    label: "PROGRAM",
    kind: 14,
    insertText: "PROGRAM ${1:ProgramName}\n\t${2:/* 代码 */}\nEND_PROGRAM",
    insertTextRules: 2,
    documentation: "定义程序",
  },
  {
    label: "FUNCTION",
    kind: 14,
    insertText:
      "FUNCTION ${1:FuncName} : ${2:BOOL}\n\tVAR_INPUT\n\t\t${3:/* 输入 */}\nEND_VAR\n\t${4:/* 代码 */}\nEND_FUNCTION",
    insertTextRules: 2,
    documentation: "定义函数",
  },
  {
    label: "FUNCTION_BLOCK",
    kind: 14,
    insertText:
      "FUNCTION_BLOCK ${1:FBName}\n\tVAR_INPUT\n\t\t${2:/* 输入 */}\nEND_VAR\n\tVAR_OUTPUT\n\t\t${3:/* 输出 */}\nEND_VAR\n\tVAR\n\t\t${4:/* 内部变量 */}\nEND_VAR\n\t${5:/* 代码 */}\nEND_FUNCTION_BLOCK",
    insertTextRules: 2,
    documentation: "定义功能块",
  },
  {
    label: "VAR",
    kind: 14,
    insertText: "VAR\n\t${1:/* 变量 */}\nEND_VAR",
    insertTextRules: 2,
    documentation: "局部变量声明",
  },
  {
    label: "VAR_INPUT",
    kind: 14,
    insertText: "VAR_INPUT\n\t${1:/* 输入变量 */}\nEND_VAR",
    insertTextRules: 2,
    documentation: "输入变量声明",
  },
  {
    label: "VAR_OUTPUT",
    kind: 14,
    insertText: "VAR_OUTPUT\n\t${1:/* 输出变量 */}\nEND_VAR",
    insertTextRules: 2,
    documentation: "输出变量声明",
  },
  {
    label: "VAR_GLOBAL",
    kind: 14,
    insertText: "VAR_GLOBAL\n\t${1:/* 全局变量 */}\nEND_VAR",
    insertTextRules: 2,
    documentation: "全局变量声明",
  },
  {
    label: "IF",
    kind: 14,
    insertText: "IF ${1:condition} THEN\n\t${2:/* 代码 */}\nEND_IF",
    insertTextRules: 2,
    documentation: "条件语句",
  },
  {
    label: "IF-ELSIF-ELSE",
    kind: 14,
    insertText:
      "IF ${1:condition} THEN\n\t${2:/* 代码 */}\nELSIF ${3:condition2} THEN\n\t${4:/* 代码 */}\nELSE\n\t${5:/* 代码 */}\nEND_IF",
    insertTextRules: 2,
    documentation: "条件分支语句",
  },
  {
    label: "CASE",
    kind: 14,
    insertText:
      "CASE ${1:variable} OF\n\t${2:value1}: ${3:/* 代码 */}\n\t${4:value2}: ${5:/* 代码 */}\nELSE\n\t${6:/* 代码 */}\nEND_CASE",
    insertTextRules: 2,
    documentation: "选择语句",
  },
  {
    label: "FOR",
    kind: 14,
    insertText:
      "FOR ${1:i} := ${2:0} TO ${3:10} DO\n\t${4:/* 代码 */}\nEND_FOR",
    insertTextRules: 2,
    documentation: "循环语句",
  },
  {
    label: "WHILE",
    kind: 14,
    insertText: "WHILE ${1:condition} DO\n\t${2:/* 代码 */}\nEND_WHILE",
    insertTextRules: 2,
    documentation: "While 循环",
  },
  {
    label: "REPEAT",
    kind: 14,
    insertText: "REPEAT\n\t${1:/* 代码 */}\nUNTIL ${2:condition}\nEND_REPEAT",
    insertTextRules: 2,
    documentation: "Repeat 循环",
  },
  { label: "RETURN", kind: 14, insertText: "RETURN", documentation: "返回" },
  { label: "EXIT", kind: 14, insertText: "EXIT", documentation: "退出循环" },
  {
    label: "CASE..OF",
    kind: 14,
    insertText: "CASE ${1:expression} OF\n\t${2:/* cases */}\nEND_CASE",
    insertTextRules: 2,
    documentation: "CASE 语句",
  },
];

// IEC 标准函数块补全
const stFunctions: any[] = [
  {
    label: "TON",
    kind: 14,
    insertText:
      "TON(\n\tIN := ${1:input},\n\tPT := T#${2:time}\n);\n${3:/* Q := ... */}",
    insertTextRules: 2,
    documentation: "接通延时定时器",
  },
  {
    label: "TOF",
    kind: 14,
    insertText:
      "TOF(\n\tIN := ${1:input},\n\tPT := T#${2:time}\n);\n${3:/* Q := ... */}",
    insertTextRules: 2,
    documentation: "断开延时定时器",
  },
  {
    label: "TP",
    kind: 14,
    insertText:
      "TP(\n\tIN := ${1:input},\n\tPT := T#${2:time}\n);\n${3:/* Q := ... */}",
    insertTextRules: 2,
    documentation: "脉冲定时器",
  },
  {
    label: "CTU",
    kind: 14,
    insertText:
      "CTU(\n\tCU := ${1:input},\n\tR := ${2:reset},\n\tPV := ${3:10}\n);\n${4:/* Q := ... */}",
    insertTextRules: 2,
    documentation: "加计数器",
  },
  {
    label: "CTD",
    kind: 14,
    insertText:
      "CTD(\n\tCD := ${1:input},\n\tLD := ${2:load},\n\tPV := ${3:10}\n);\n${4:/* Q := ... */}",
    insertTextRules: 2,
    documentation: "减计数器",
  },
  {
    label: "CTUD",
    kind: 14,
    insertText:
      "CTUD(\n\tCU := ${1:input},\n\tCD := ${2:input2},\n\tR := ${3:reset},\n\tLD := ${4:load},\n\tPV := ${5:10}\n);\n${6:/* QU := ... */}",
    insertTextRules: 2,
    documentation: "双向计数器",
  },
  {
    label: "SR",
    kind: 14,
    insertText:
      "SR(\n\tS1 := ${1:set},\n\tR := ${2:reset}\n);\n${3:/* Q := ... */}",
    insertTextRules: 2,
    documentation: "优先置位 SR",
  },
  {
    label: "RS",
    kind: 14,
    insertText:
      "RS(\n\tS := ${1:set},\n\tR1 := ${2:reset}\n);\n${3:/* Q := ... */}",
    insertTextRules: 2,
    documentation: "优先复位 RS",
  },
  {
    label: "R_TRIG",
    kind: 14,
    insertText: "R_TRIG(\n\tCLK := ${1:input}\n);\n${2:/* Q := ... */}",
    insertTextRules: 2,
    documentation: "上升沿检测",
  },
  {
    label: "F_TRIG",
    kind: 14,
    insertText: "F_TRIG(\n\tCLK := ${1:input}\n);\n${2:/* Q := ... */}",
    insertTextRules: 2,
    documentation: "下降沿检测",
  },
];

// 类型补全
const stTypes: any[] = [
  { label: "BOOL", kind: 17, insertText: "BOOL", documentation: "布尔类型" },
  { label: "INT", kind: 17, insertText: "INT", documentation: "16位整数" },
  { label: "DINT", kind: 17, insertText: "DINT", documentation: "32位整数" },
  { label: "LINT", kind: 17, insertText: "LINT", documentation: "64位整数" },
  { label: "REAL", kind: 17, insertText: "REAL", documentation: "32位浮点" },
  { label: "LREAL", kind: 17, insertText: "LREAL", documentation: "64位浮点" },
  { label: "TIME", kind: 17, insertText: "TIME", documentation: "时间类型" },
  {
    label: "STRING",
    kind: 17,
    insertText: "STRING",
    documentation: "字符串类型",
  },
];

// 数学函数补全
const stMathFunctions: any[] = [
  {
    label: "ABS",
    kind: 3,
    insertText: "ABS(${1:value})",
    insertTextRules: 2,
    documentation: "绝对值",
  },
  {
    label: "SQRT",
    kind: 3,
    insertText: "SQRT(${1:value})",
    insertTextRules: 2,
    documentation: "平方根",
  },
  {
    label: "SIN",
    kind: 3,
    insertText: "SIN(${1:value})",
    insertTextRules: 2,
    documentation: "正弦",
  },
  {
    label: "COS",
    kind: 3,
    insertText: "COS(${1:value})",
    insertTextRules: 2,
    documentation: "余弦",
  },
  {
    label: "TAN",
    kind: 3,
    insertText: "TAN(${1:value})",
    insertTextRules: 2,
    documentation: "正切",
  },
  {
    label: "ASIN",
    kind: 3,
    insertText: "ASIN(${1:value})",
    insertTextRules: 2,
    documentation: "反正弦",
  },
  {
    label: "ACOS",
    kind: 3,
    insertText: "ACOS(${1:value})",
    insertTextRules: 2,
    documentation: "反余弦",
  },
  {
    label: "ATAN",
    kind: 3,
    insertText: "ATAN(${1:value})",
    insertTextRules: 2,
    documentation: "反正切",
  },
  {
    label: "LN",
    kind: 3,
    insertText: "LN(${1:value})",
    insertTextRules: 2,
    documentation: "自然对数",
  },
  {
    label: "LOG",
    kind: 3,
    insertText: "LOG(${1:value})",
    insertTextRules: 2,
    documentation: "常用对数",
  },
  {
    label: "EXP",
    kind: 3,
    insertText: "EXP(${1:value})",
    insertTextRules: 2,
    documentation: "指数",
  },
  {
    label: "EXPT",
    kind: 3,
    insertText: "EXPT(${1:base}, ${2:exponent})",
    insertTextRules: 2,
    documentation: "幂运算",
  },
];

// 数学运算符补全
const stOperators: any[] = [
  {
    label: ":=",
    kind: 13,
    insertText: ":= ${1:value}",
    insertTextRules: 2,
    documentation: "赋值",
  },
  { label: "AND", kind: 13, insertText: "AND", documentation: "逻辑与" },
  { label: "OR", kind: 13, insertText: "OR", documentation: "逻辑或" },
  { label: "XOR", kind: 13, insertText: "XOR", documentation: "逻辑异或" },
  { label: "NOT", kind: 13, insertText: "NOT", documentation: "逻辑非" },
  { label: "MOD", kind: 13, insertText: "MOD", documentation: "取模" },
];

// 注册自动补全提供者
export function registerSTCompletion(monacoInstance: any) {
  monacoInstance.languages.registerCompletionItemProvider("st", {
    provideCompletionItems: (model: any, position: any) => {
      const word = model.getWordUntilPosition(position);
      const range = {
        startLineNumber: position.lineNumber,
        startColumn: word.startColumn,
        endLineNumber: position.lineNumber,
        endColumn: word.endColumn,
      };

      return {
        suggestions: [
          ...stKeywords.map((item) => ({ ...item, range })),
          ...stFunctions.map((item) => ({ ...item, range })),
          ...stTypes.map((item) => ({ ...item, range })),
          ...stMathFunctions.map((item) => ({ ...item, range })),
          ...stOperators.map((item) => ({ ...item, range })),
        ],
      };
    },
  });
}
