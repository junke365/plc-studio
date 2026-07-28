// 编辑器类型定义

// 编辑器语言
export enum EditorLanguage {
  LD = "LD", // 梯形图
  FBD = "FBD", // 功能块图
  ST = "ST", // 结构化文本
  IL = "IL", // 指令表
  SFC = "SFC", // 顺序功能图
  XML = "XML", // XML 编辑
  HMI = "HMI", // HMI 设计器
  Kinematics = "Kinematics", // 运动学配置
  Text = "Text", // 纯文本
}

// 编辑器标签
export interface EditorTab {
  id: string;
  title: string;
  language: EditorLanguage;
  path: string;
  modified: boolean;
  content: string;
  pouName?: string;
}

// 图形元素类型
export enum GraphicElementType {
  // LD 元素
  Contact = "contact",
  ContactNormallyOpen = "contact_nopen",
  ContactNormallyClosed = "contact_nclose",
  Coil = "coil",
  CoilSet = "coil_set",
  CoilReset = "coil_reset",
  CoilNormal = "coil_normal",
  TimerOnDelay = "timer_ondelay",
  TimerOffDelay = "timer_offdelay",
  CounterUp = "counter_up",
  CounterDown = "counter_down",
  CounterUpDown = "counter_updown",

  // FBD 元素
  FunctionBlock = "function_block",
  Function = "function",
  Input = "input",
  Output = "output",
  Variable = "variable",
  Constant = "constant",
  Wire = "wire",

  // SFC 元素
  Step = "step",
  Transition = "transition",
  Action = "action",
  InitialStep = "initial_step",
  Divergence = "divergence",
  Convergence = "convergence",
  Jump = "jump",
}

// 图形元素
export interface GraphicElement {
  id: string;
  type: GraphicElementType;
  x: number;
  y: number;
  width: number;
  height: number;
  rotation?: number;
  scale?: number;
  label?: string;
  variable?: string;
  parameters?: Record<string, unknown>;
  connections?: string[];
}

// 画布状态
export interface CanvasState {
  zoom: number;
  panX: number;
  panY: number;
  selectedElementId?: string;
  elements: GraphicElement[];
}

// 编辑器工具栏按钮
export interface ToolbarButton {
  id: string;
  icon: string;
  label: string;
  tooltip: string;
  enabled: boolean;
  action: () => void;
}

// 项目树节点
export interface ProjectTreeNode {
  id: string;
  label: string;
  type: string;
  icon?: string;
  children?: ProjectTreeNode[];
  expanded?: boolean;
  data?: unknown;
}

// 搜索结果
export interface SearchResult {
  file: string;
  line: number;
  column: number;
  match: string;
  context: string;
}

// 输出面板消息
export interface OutputMessage {
  timestamp: Date;
  level: "info" | "warning" | "error" | "debug";
  source: string;
  message: string;
}
