// IEC 61131-3 ST 语言配置 — Monarch tokenizer + 折叠规则

// ST 语言注册
export function registerSTLanguage(monacoInstance: any) {
  // 注册 ST 语言
  monacoInstance.languages.register({
    id: "st",
    extensions: ["st", "ST", " structuredtext"],
  });

  // Monarch tokenizer（语法高亮）
  monacoInstance.languages.setMonarchTokensProvider("st", {
    // 关键字
    keywords: [
      "PROGRAM",
      "END_PROGRAM",
      "FUNCTION",
      "END_FUNCTION",
      "FUNCTION_BLOCK",
      "END_FUNCTION_BLOCK",
      "VAR",
      "VAR_INPUT",
      "VAR_OUTPUT",
      "VAR_IN_OUT",
      "VAR_GLOBAL",
      "VAR_EXTERNAL",
      "VAR_TEMP",
      "END_VAR",
      "VAR_ACCESS",
      "VAR_CONFIG",
      "VAR_INTERVAL",
      "VAR_PERMANENT",
      "CONSTANT",
      "RETAIN",
      "NON_RETAIN",
      "IF",
      "THEN",
      "ELSIF",
      "ELSE",
      "END_IF",
      "CASE",
      "OF",
      "END_CASE",
      "FOR",
      "TO",
      "BY",
      "DO",
      "END_FOR",
      "WHILE",
      "END_WHILE",
      "REPEAT",
      "UNTIL",
      "END_REPEAT",
      "RETURN",
      "EXIT",
      "AND",
      "OR",
      "XOR",
      "NOT",
      "MOD",
      "TRUE",
      "FALSE",
      "TYPE",
      "END_TYPE",
      "STRUCT",
      "END_STRUCT",
      "ARRAY",
      "AT",
      "CONFIGURATION",
      "END_CONFIGURATION",
      "RESOURCE",
      "END_RESOURCE",
      "TASK",
      "END_TASK",
      "WITH",
      "INITIAL_STEP",
      "END_STEP",
      "TRANSITION",
      "END_TRANSITION",
      "ACTION",
      "END_ACTION",
    ],

    // 类型
    typeKeywords: [
      "BOOL",
      "BYTE",
      "WORD",
      "DWORD",
      "LWORD",
      "SINT",
      "INT",
      "DINT",
      "LINT",
      "USINT",
      "UINT",
      "UDINT",
      "ULINT",
      "REAL",
      "LREAL",
      "TIME",
      "DATE",
      "TIME_OF_DAY",
      "TOD",
      "DATE_AND_TIME",
      "DT",
      "STRING",
      "WSTRING",
    ],

    // IEC 标准函数块
    builtins: [
      "ABS",
      "SQRT",
      "LN",
      "LOG",
      "EXP",
      "SIN",
      "COS",
      "TAN",
      "ASIN",
      "ACOS",
      "ATAN",
      "ADD",
      "SUB",
      "MUL",
      "DIV",
      "MOD",
      "GT",
      "GE",
      "LT",
      "LE",
      "EQ",
      "NE",
      "SHL",
      "SHR",
      "ROL",
      "ROR",
      "LEN",
      "LEFT",
      "RIGHT",
      "MID",
      "CONCAT",
      "INSERT",
      "DELETE",
      "REPLACE",
      "FIND",
      "SEL",
      "MAX",
      "MIN",
      "LIMIT",
      "MUX",
      "CTU",
      "CTD",
      "CTUD",
      "TP",
      "TON",
      "TOF",
      "SR",
      "RS",
      "R_TRIG",
      "F_TRIG",
      "MOVE",
      "CONVERT",
      "ROUND",
      "TRUNC",
      "SCALE",
      "NORM",
    ],

    operators: [
      ":=",
      "=",
      "<>",
      "<",
      ">",
      "<=",
      ">=",
      "+",
      "-",
      "*",
      "/",
      "**",
      "&",
      "#",
      "$",
    ],

    symbols: /[=><!~?:&|+\-*/^%]+/,
    digits: /\d+(_+\d+)*/,
    tokenizer: {
      root: [
        // 行注释
        [/\/\/.*$/, "comment"],
        // 块注释
        [/\(\*/, "comment", "@comment"],

        // 字符串
        [/'/, "string", "@string"],
        [/"$/, "string", "@wstring"],

        // 标识符和关键字
        [
          /[a-zA-Z_]\w*/,
          {
            cases: {
              "@keywords": "keyword",
              "@typeKeywords": "type",
              "@builtins": "predefined",
              "@default": "identifier",
            },
          },
        ],

        // 数字
        [/\d+\.\d+([eE][-+]?\d+)?/, "number.float"],
        [/\d#[0-9A-Fa-f_]+/, "number.hex"],
        [/\d+/, "number"],

        // 符号
        [/[():\[\]{},;.]/, "@brackets"],
        [
          /@symbols/,
          {
            cases: {
              "@operators": "operator",
              "@default": "",
            },
          },
        ],

        // 空白
        [/\s+/, "white"],
      ],

      comment: [
        [/[^(*]+/, "comment"],
        [/\*\)/, "comment", "@pop"],
        [/[(*]/, "comment"],
      ],

      string: [
        [/[^']+/, "string"],
        [/''/, "string.escape"],
        [/'/, "string", "@pop"],
      ],

      wstring: [
        [/[^"]+/, "string"],
        [/""/, "string.escape"],
        [/"$/, "string", "@pop"],
      ],
    },
  } as any);

  // 语言配置（括号匹配、自动闭合、缩进规则）
  monacoInstance.languages.setLanguageConfiguration("st", {
    comments: {
      lineComment: "//",
      blockComment: ["(*", "*)"],
    },
    brackets: [
      ["{", "}"],
      ["[", "]"],
      ["(", ")"],
    ],
    autoClosingPairs: [
      { open: "{", close: "}" },
      { open: "[", close: "]" },
      { open: "(", close: ")" },
      { open: "'", close: "'", notIn: ["string"] },
      { open: '"', close: '"', notIn: ["string"] },
    ],
    surroundingPairs: [
      { open: "{", close: "}" },
      { open: "[", close: "]" },
      { open: "(", close: ")" },
      { open: "'", close: "'" },
      { open: '"', close: '"' },
    ],
    folding: {
      markers: {
        start:
          /^\s*(PROGRAM|FUNCTION|FUNCTION_BLOCK|VAR|VAR_INPUT|VAR_OUTPUT|IF|FOR|WHILE|REPEAT|CASE|STRUCT|TYPE)\b/i,
        end: /^\s*(END_PROGRAM|END_FUNCTION|END_FUNCTION_BLOCK|END_VAR|END_IF|END_FOR|END_WHILE|END_REPEAT|END_CASE|END_STRUCT|END_TYPE)\b/i,
      },
    },
    indentationRules: {
      increaseIndentPattern:
        /^\s*(PROGRAM|FUNCTION|FUNCTION_BLOCK|VAR|VAR_INPUT|VAR_OUTPUT|VAR_IN_OUT|IF|FOR|WHILE|REPEAT|CASE|STRUCT|TYPE|ELSE|ELSIF)\b/i,
      decreaseIndentPattern:
        /^\s*(END_PROGRAM|END_FUNCTION|END_FUNCTION_BLOCK|END_VAR|END_IF|END_FOR|END_WHILE|END_REPEAT|END_CASE|END_STRUCT|END_TYPE|ELSE|ELSIF)\b/i,
    },
  });
}
