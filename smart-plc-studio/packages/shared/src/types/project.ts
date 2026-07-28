// 项目结构类型定义

import type { IecType } from './plc'

// POU 类型
export enum PouType {
  Program = 'program',
  FunctionBlock = 'functionBlock',
  Function = 'function'
}

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

// 变量定义
export interface Variable {
  name: string
  type: string
  className: VarClass
  location?: string
  initialValue?: unknown
  comment?: string
  address?: string
}

// POU 变量表
export interface PouVarTable {
  inputVars: Variable[]
  outputVars: Variable[]
  inOutVars: Variable[]
  localVars: Variable[]
  tempVars: Variable[]
  globalVars: Variable[]
  externalVars: Variable[]
  accessVars: Variable[]
}

// POU 定义
export interface POU {
  name: string
  pouType: PouType
  variables: PouVarTable
  body: string
  bodyLanguage?: string
  comment?: string
}

// 过渡定义
export interface Transition {
  name: string
  body: string
  bodyLanguage?: string
  variable?: string
  condition?: string
  comment?: string
}

// 动作定义
export interface Action {
  name: string
  body: string
  bodyLanguage?: string
  comment?: string
}

// 资源定义
export interface Resource {
  name: string
  tasks: Task[]
  programs: ProgramInstance[]
  variables?: Variable[]
}

// 任务定义
export interface Task {
  name: string
  interval?: string
  priority?: number
  single?: boolean
}

// 程序实例
export interface ProgramInstance {
  name: string
  typeName: string
  taskName?: string
  variables?: Variable[]
}

// 配置定义
export interface Configuration {
  name: string
  resources: Resource[]
  variables?: Variable[]
  comment?: string
}

// 数据类型定义
export interface DataType {
  name: string
  baseType: string
  initialValue?: unknown
  comment?: string
  elements?: DataTypeElement[]
}

export interface DataTypeElement {
  name: string
  value: number
  comment?: string
}

// PLC 项目
export interface PlcProject {
  name: string
  path: string
  pous: POU[]
  dataTypes: DataType[]
  configurations: Configuration[]
  fileHeader?: string
  contentHeader?: string
}

// 项目节点
export interface ProjectNode {
  id: string
  name: string
  type: ProjectNodeType
  children?: ProjectNode[]
  data?: unknown
}

export enum ProjectNodeType {
  Project = 'project',
  DataTypes = 'dataTypes',
  Functions = 'functions',
  FunctionBlocks = 'functionBlocks',
  Programs = 'programs',
  POU = 'pou',
  DataType = 'dataType',
  Configurations = 'configurations',
  Configuration = 'configuration',
  Resources = 'resources',
  Resource = 'resource',
  Transitions = 'transitions',
  Transition = 'transition',
  Actions = 'actions',
  Action = 'action',
  Properties = 'properties'
}
