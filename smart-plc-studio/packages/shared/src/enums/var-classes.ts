// 变量类别枚举

// 变量类别
export enum VarClass {
  Local = 'Local',
  Temp = 'Temp',
  Input = 'Input',
  Output = 'Output',
  InOut = 'InOut',
  External = 'External',
  Global = 'Global',
  Access = 'Access'
}

// 搜索结果变量类型
export const SearchResultVarTypes: Record<string, string> = {
  inputVars: 'var_input',
  outputVars: 'var_output',
  inOutVars: 'var_inout'
}

// 变量显示顺序
export const VarOrder: VarClass[] = [
  VarClass.Local,
  VarClass.Temp,
  VarClass.Input,
  VarClass.Output,
  VarClass.InOut,
  VarClass.External,
  VarClass.Global,
  VarClass.Access
]

// 变量类别中文名
export const VarClassNames: Record<VarClass, string> = {
  [VarClass.Local]: '局部变量',
  [VarClass.Temp]: '临时变量',
  [VarClass.Input]: '输入变量',
  [VarClass.Output]: '输出变量',
  [VarClass.InOut]: '输入输出变量',
  [VarClass.External]: '外部变量',
  [VarClass.Global]: '全局变量',
  [VarClass.Access]: '访问变量'
}

// 变量类别图标
export const VarClassIcons: Record<VarClass, string> = {
  [VarClass.Local]: 'folder',
  [VarClass.Temp]: 'clock',
  [VarClass.Input]: 'arrow-right',
  [VarClass.Output]: 'arrow-left',
  [VarClass.InOut]: 'arrows-exchange',
  [VarClass.External]: 'globe',
  [VarClass.Global]: 'globe',
  [VarClass.Access]: 'lock'
}
