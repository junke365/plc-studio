// ROS 2 功能块统一元数据：FBD 内置库与全局库管理器共用，保证两侧同步。
export interface Ros2LibItem {
  name: string;
  label: string;
  desc: string;
  icon: string;
  emoji: string;
  category: "ROS2";
}

export const ROS2_LIB: Ros2LibItem[] = [
  {
    name: "ROS2_PUBLISH",
    label: "发布话题",
    desc: "把 JSON 消息发布到 ROS 2 话题（走后端 /api/ros2/publish）",
    icon: "send",
    emoji: "📤",
    category: "ROS2",
  },
  {
    name: "ROS2_SUBSCRIBE",
    label: "订阅话题",
    desc: "实时订阅 ROS 2 话题，新消息输出到 DATA 引脚",
    icon: "rss_feed",
    emoji: "📥",
    category: "ROS2",
  },
  {
    name: "ROS2_SERVICE",
    label: "调用服务",
    desc: "调用 ROS 2 服务并输出响应（走后端 /api/ros2/call）",
    icon: "call_split",
    emoji: "🛰️",
    category: "ROS2",
  },
];
