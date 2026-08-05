// rclnodejs 是 optionalDependency：仅在安装 ROS2 的 Linux/macOS 上存在。
// 此处仅声明模块存在，运行时用动态 import 并捕获失败。
declare module 'rclnodejs' {
  export function init(): Promise<void>
  export function isInitialized(): boolean
  export class Node {
    constructor(name: string)
  }
  const _default: any
  export default _default
}
