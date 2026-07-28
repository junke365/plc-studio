// POU 类型枚举（参考 Python 版 types_enums.py）

// 项目可编辑项
export enum ItemEditable {
  Project = 0,
  POU = 1,
  Variable = 2,
  Transition = 3,
  Action = 4,
  Configuration = 5,
  Resource = 6,
  DataType = 7
}

// 项目不可编辑项
export enum ItemUneditable {
  DataTypes = 8,
  Function = 9,
  FunctionBlock = 10,
  Program = 11,
  Transitions = 12,
  Actions = 13,
  Configurations = 14,
  Resources = 15,
  Properties = 16
}

// 变量项
export enum ItemVariable {
  Local = 17,
  Global = 18,
  External = 19,
  Temp = 20,
  Input = 21,
  Output = 22,
  InOut = 23
}

// 配置节点项
export const ItemConfNode = 25

// 变量类别信息
export const VarClassInfos: Record<string, { varTable: string; item: number }> = {
  Local: { varTable: 'localVars', item: ItemVariable.Local },
  Global: { varTable: 'globalVars', item: ItemVariable.Global },
  External: { varTable: 'externalVars', item: ItemVariable.External },
  Temp: { varTable: 'tempVars', item: ItemVariable.Temp },
  Input: { varTable: 'inputVars', item: ItemVariable.Input },
  Output: { varTable: 'outputVars', item: ItemVariable.Output },
  InOut: { varTable: 'inOutVars', item: ItemVariable.InOut }
}

// POU 类型映射
export const PouTypeMap: Record<string, number> = {
  program: ItemEditable.Program,
  functionBlock: ItemEditable.POU,
  function: ItemEditable.POU
}

// 类别类型映射
export const ClassTypeMap: Record<string, number> = {
  configuration: ItemEditable.Configuration,
  resource: ItemEditable.Resource,
  action: ItemEditable.Action,
  transition: ItemEditable.Transition,
  program: ItemEditable.Program
}

// 位置项
export enum LocationItem {
  ConfNode = 0,
  Module = 1,
  Group = 2,
  VarInput = 3,
  VarOutput = 4,
  VarMemory = 5
}

// 不可编辑名称
export const UneditableNames = [
  '用户定义 POU',
  '函数',
  '功能块',
  '程序',
  '数据类型',
  '过渡',
  '动作',
  '配置',
  '资源',
  '属性'
]

// 树节点标签
export enum TreeNodeLabel {
  UserDefinedPOUs = 0,
  Functions = 1,
  FunctionBlocks = 2,
  Programs = 3,
  DataTypes = 4,
  Transitions = 5,
  Actions = 6,
  Configurations = 7,
  Resources = 8,
  Properties = 9
}

// 变量类型名称
export const VarTypeNames: Record<string, string> = {
  Local: '局部变量',
  Temp: '临时变量',
  Input: '输入变量',
  Output: '输出变量',
  InOut: '输入输出变量',
  External: '外部变量',
  Global: '全局变量',
  Access: '访问变量'
}

// POU 类型名称
export const PouTypeNames: Record<string, string> = {
  program: '程序',
  functionBlock: '功能块',
  function: '函数'
}
