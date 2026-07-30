<template>
  <header class="top-nav" @click.stop>
    <div class="nav-left">
      <span class="app-brand">PLC Studio</span>
      <nav class="nav-menu">
        <div v-for="item in menuItems" :key="item.id" class="nav-menu-wrapper">
          <button
            class="nav-menu-item"
            :class="{ active: activeMenu === item.id }"
            @click.stop="toggleMenu(item.id)"
            @mouseenter="hoverMenu(item.id)"
          >
            {{ item.label }}
          </button>
          <!-- 下拉菜单 -->
          <div v-if="activeMenu === item.id" class="dropdown-menu" @click.stop>
            <template
              v-for="(entry, idx) in getMenuEntries(item.id)"
              :key="idx"
            >
              <div v-if="entry.divider" class="dropdown-divider" />
              <!-- 普通菜单项 -->
              <div
                v-else-if="!entry.submenu"
                class="dropdown-item"
                :class="{ disabled: entry.disabled }"
                @click.stop="executeAction(entry)"
              >
                <span class="dropdown-check-space">
                  <span
                    v-if="entry.checked !== undefined"
                    class="material-symbols-outlined"
                    :style="{ color: 'var(--primary)' }"
                    >{{ entry.checked ? "done" : "" }}</span
                  >
                </span>
                <span class="dropdown-label">{{ entry.label }}</span>
                <span v-if="entry.shortcut" class="dropdown-shortcut">{{
                  entry.shortcut
                }}</span>
                <span
                  v-if="entry.arrow"
                  class="material-symbols-outlined dropdown-arrow"
                  >chevron_right</span
                >
              </div>
              <!-- 带子菜单的项 -->
              <div
                v-else
                class="dropdown-item dropdown-parent"
                @mouseenter="activeSubmenu = entry.submenu"
                @mouseleave="activeSubmenu = null"
              >
                <span class="dropdown-check-space" />
                <span class="dropdown-label">{{ entry.label }}</span>
                <span class="material-symbols-outlined dropdown-arrow"
                  >chevron_right</span
                >
                <!-- 子菜单 -->
                <div
                  v-if="activeSubmenu === entry.submenu"
                  class="dropdown-submenu"
                  @click.stop
                >
                  <template
                    v-for="(sub, sIdx) in submenuItems[entry.submenu!] || []"
                    :key="sIdx"
                  >
                    <div v-if="sub.divider" class="dropdown-divider" />
                    <div
                      v-else
                      class="dropdown-item"
                      :class="{ disabled: sub.disabled }"
                      @click.stop="executeAction(sub)"
                    >
                      <span class="dropdown-check-space" />
                      <span class="dropdown-label">{{ sub.label }}</span>
                      <span v-if="sub.shortcut" class="dropdown-shortcut">{{
                        sub.shortcut
                      }}</span>
                    </div>
                  </template>
                </div>
              </div>
            </template>
          </div>
        </div>
      </nav>
    </div>
    <div class="nav-right">
      <div class="search-box">
        <span class="material-symbols-outlined">search</span>
        <input placeholder="搜索资源..." />
      </div>
      <button class="btn-icon" title="设置" @click="settingsVisible = true">
        <span class="material-symbols-outlined">settings</span>
      </button>
      <button class="btn-icon" title="窗口布局">
        <span class="material-symbols-outlined">dashboard</span>
      </button>
      <button class="btn-run" title="运行仿真">
        <span
          class="material-symbols-outlined"
          style="font-variation-settings: &quot;FILL&quot; 1"
          >play_arrow</span
        >
      </button>
    </div>
    <SettingsDialog v-model:visible="settingsVisible" />
    <input ref="fileInput" type="file" accept=".plcproj,.json" style="display:none" @change="onFileSelected" />
  </header>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from "vue";
import { useRouter } from "vue-router";
import { useUiStore } from "../stores/ui";
import { useProjectStore } from "../stores/project";
import { useEditorStore } from "../stores/editor";
import { useDebugStore } from "../stores/debug";
import { useSettingsStore, type RuntimeTarget } from "../stores/settings";
import type { PlcProject } from "@smart-plc/shared";
import SettingsDialog from "../components/dialogs/SettingsDialog.vue";

const settingsVisible = ref(false)
const fileInput = ref<HTMLInputElement | null>(null)

const router = useRouter();
const uiStore = useUiStore();
const projectStore = useProjectStore();
const editorStore = useEditorStore();
const debugStore = useDebugStore();
const settingsStore = useSettingsStore();

const activeMenu = ref<string | null>(null);
const activeSubmenu = ref<string | null>(null);

// 菜单栏项目
const menuItems = [
  { id: "file", label: "文件" },
  { id: "edit", label: "编辑" },
  { id: "view", label: "视图" },
  { id: "project", label: "工程" },
  { id: "compile", label: "编译" },
  { id: "online", label: "在线" },
  { id: "debug", label: "调试" },
  { id: "simulator", label: "仿真器" },
  { id: "tools", label: "工具" },
  { id: "window", label: "窗口" },
  { id: "help", label: "帮助" },
];

// 菜单项定义
interface MenuItem {
  label: string;
  shortcut?: string;
  icon?: string;
  disabled?: boolean;
  checked?: boolean;
  divided?: boolean;
  divider?: boolean;
  submenu?: string;
  action?: () => void;
}

// 获取菜单项
function getMenuEntries(menuId: string): MenuItem[] {
  switch (menuId) {
    case "file":
      return fileMenuEntries;
    case "edit":
      return editMenuEntries;
    case "view":
      return viewMenuEntries;
    case "project":
      return projectMenuEntries;
    case "compile":
      return compileMenuEntries;
    case "online":
      return onlineMenuEntries.value;
    case "debug":
      return debugMenuEntries;
    case "simulator":
      return simulatorMenuEntries;
    case "tools":
      return toolsMenuEntries;
    case "window":
      return windowMenuEntries;
    case "help":
      return helpMenuEntries;
    default:
      return [];
  }
}

// ==================== 文件菜单 ====================
const fileMenuEntries: MenuItem[] = [
  { label: "新建项目", shortcut: "Ctrl+N", action: handleNewProject },
  { label: "打开项目", shortcut: "Ctrl+O", action: handleOpenProject },
  { label: "保存项目", shortcut: "Ctrl+S", action: handleSaveProject },
  { label: "另存为...", shortcut: "Ctrl+Shift+S", action: handleSaveProjectAs },
  { divider: true, label: "" },
  { label: "最近项目", arrow: true, submenu: "recentProjects" },
  { divider: true, label: "" },
  { label: "示例项目", arrow: true, submenu: "sampleProjects" },
  { divider: true, label: "" },
  { label: "导入", arrow: true, submenu: "import" },
  { divider: true, label: "" },
  { label: "退出", action: handleExit },
];

// 子菜单（最近项目动态生成）
const submenuItems = computed<Record<string, MenuItem[]>>(() => ({
  // 最近项目子菜单
  recentProjects:
    projectStore.recentProjects.length > 0
      ? [
          ...projectStore.recentProjects.map((p) => ({
            label: p.name,
            action: () => {
              const saved = JSON.parse(localStorage.getItem("plc-projects") || "{}")
              if (saved[p.path]) {
                projectStore.openProject(p.path, saved[p.path])
                debugStore.addLog("info", `已打开最近项目: ${p.name}`)
              } else {
                debugStore.addLog("warning", `最近项目不存在: ${p.path}`)
              }
            },
          })),
          { divider: true, label: "" },
          {
            label: "清除最近项目列表",
            action: () => projectStore.clearRecentProjects(),
          },
        ]
      : [{ label: "(无最近项目)", disabled: true }],
  // 导入子菜单
  import: [
    { label: "导入 PLC 程序...", action: () => importFile(".st", "ST 程序文件") },
    { label: "导入 XML 文件...", action: () => importFile(".xml", "XML 文件") },
    { label: "导入硬件配置...", action: () => importFile(".xml", "硬件配置 XML") },
  ],

  // 示例项目子菜单
  sampleProjects: [
    { label: "01 位逻辑", action: () => handleOpenSampleProject("01_basic_logic") },
    { label: "02 定时器/计数器", action: () => handleOpenSampleProject("02_timer_counter") },
    { label: "03 模拟量处理", action: () => handleOpenSampleProject("03_analog_processing") },
    { label: "04 状态机", action: () => handleOpenSampleProject("04_state_machine") },
    { label: "05 运动控制", action: () => handleOpenSampleProject("05_motion_control") },
  ],

  // UART 调试子菜单
  uartDebug: [
    { label: "串口终端", action: () => openToolPanel("uart-terminal") },
    { divider: true, label: "" },
    { label: "UART 波形查看器", action: () => openToolPanel("uart-waveform") },
    { label: "UART 数据记录", action: () => openToolPanel("uart-logger") },
    { divider: true, label: "" },
    { label: "UART 配置", action: () => openToolPanel("uart-config") },
  ],

  // Modbus 调试子菜单
  modbusDebug: [
    { label: "Modbus RTU 调试器", action: () => openToolPanel("modbus-rtu") },
    { label: "Modbus TCP 调试器", action: () => openToolPanel("modbus-tcp") },
    { divider: true, label: "" },
    { label: "Modbus 数据监控", action: () => openToolPanel("modbus-monitor") },
    { label: "Modbus 寄存器图表", action: () => openToolPanel("modbus-chart") },
    { divider: true, label: "" },
    { label: "Modbus 协议分析", action: () => openToolPanel("modbus-analyzer") },
  ],

  // CAN / CANopen 调试子菜单
  canDebug: [
    { label: "CAN 总线监控", action: () => openToolPanel("can-monitor") },
    { label: "CAN 帧发送器", action: () => openToolPanel("can-sender") },
    { divider: true, label: "" },
    { label: "CANopen 网络管理", action: () => openToolPanel("canopen-nmt") },
    { label: "CANopen SDO 调试", action: () => openToolPanel("canopen-sdo") },
    { label: "CANopen PDO 监控", action: () => openToolPanel("canopen-pdo") },
    { divider: true, label: "" },
    { label: "CAN 报文图表", action: () => openToolPanel("can-chart") },
  ],

  // 网络调试子菜单
  networkDebug: [
    { label: "TCP 服务端", action: () => openToolPanel("tcp-server") },
    { label: "TCP 客户端", action: () => openToolPanel("tcp-client") },
    { divider: true, label: "" },
    { label: "UDP 服务端", action: () => openToolPanel("udp-server") },
    { label: "UDP 客户端", action: () => openToolPanel("udp-client") },
    { divider: true, label: "" },
    { label: "WebSocket 调试", action: () => openToolPanel("websocket-debug") },
    { label: "HTTP 请求测试", action: () => openToolPanel("http-tester") },
    { divider: true, label: "" },
    { label: "网络数据图表", action: () => openToolPanel("network-chart") },
  ],

  // 电机调试子菜单
  motorDebug: [
    { label: "PID 调谐器", action: () => openToolPanel("pid-tuner") },
    { label: "电机响应曲线", action: () => openToolPanel("motor-response") },
    { divider: true, label: "" },
    { label: "步进电机调试", action: () => openToolPanel("stepper-debug") },
    { label: "伺服电机调试", action: () => openToolPanel("servo-debug") },
    { label: "变频器调试", action: () => openToolPanel("vfd-debug") },
    { divider: true, label: "" },
    { label: "运动轨迹图表", action: () => openToolPanel("motion-chart") },
    { label: "扭矩/速度监控", action: () => openToolPanel("torque-monitor") },
  ],
}));

// ==================== 编辑菜单 ====================
const editMenuEntries: MenuItem[] = [
  {
    label: "撤消",
    shortcut: "Ctrl+Z",
    disabled: !editorStore.canUndo,
    action: handleUndo,
  },
  {
    label: "重做",
    shortcut: "Ctrl+Y",
    disabled: !editorStore.canRedo,
    action: handleRedo,
  },
  { divider: true, label: "" },
  { label: "剪切", shortcut: "Ctrl+X", action: handleCut },
  { label: "拷贝", shortcut: "Ctrl+C", action: handleCopy },
  { label: "粘贴", shortcut: "Ctrl+V", action: handlePaste },
  { divider: true, label: "" },
  { label: "全选", shortcut: "Ctrl+A", action: handleSelectAll },
  { divider: true, label: "" },
  { label: "查找", shortcut: "Ctrl+F", action: handleFind },
  { label: "替换", shortcut: "Ctrl+H", action: handleReplace },
];

// ==================== 视图菜单 ====================
const viewMenuEntries: MenuItem[] = [
  {
    label: "活动栏",
    checked: uiStore.activityBarVisible,
    action: () => toggleView("activityBar"),
  },
  {
    label: "侧边栏",
    checked: uiStore.sidebarVisible,
    action: () => toggleView("sidebar"),
  },
  { divider: true, label: "" },
  {
    label: "底部面板",
    checked: uiStore.bottomPanelVisible,
    action: () => toggleView("bottomPanel"),
  },
  {
    label: "状态栏",
    checked: uiStore.statusBarVisible,
    action: () => toggleView("statusBar"),
  },
  { divider: true, label: "" },
  { label: "重置布局", icon: "restart_alt", action: resetLayout },
];

// ==================== 工程菜单 ====================
const projectMenuEntries: MenuItem[] = [
  { label: "新建工程", action: () => router.push('/welcome') },
  { divider: true, label: "" },
  { label: "工程设置...", action: () => logAction("工程设置") },
  { divider: true, label: "" },
  { label: "添加POU", action: () => logAction("添加POU") },
  { label: "添加程序组织单元", action: () => logAction("添加程序组织单元") },
  { label: "添加全局变量", action: () => logAction("添加全局变量") },
  { divider: true, label: "" },
  {
    label: "编译全部",
    shortcut: "Ctrl+F7",
    action: () => logAction("编译全部"),
  },
];

// ==================== 编译菜单 ====================
const compileMenuEntries: MenuItem[] = [
  {
    label: "编译当前文件",
    shortcut: "F7",
    action: () => logAction("编译当前文件"),
  },
  {
    label: "编译全部",
    shortcut: "Ctrl+F7",
    action: () => logAction("编译全部"),
  },
  { divider: true, label: "" },
  { label: "清理编译输出", action: () => logAction("清理编译输出") },
  { label: "查看编译日志", action: () => logAction("查看编译日志") },
];

// ==================== 在线菜单 ====================
const targetLabels: Record<RuntimeTarget, string> = {
  "stm32": "STM32",
  "esp32": "ESP32",
  "linux-arm": "Linux ARM",
  "linux-x86": "Linux x86",
}

const onlineMenuEntries = computed<MenuItem[]>(() => {
  const items: MenuItem[] = [
    { label: "连接设备...", action: handleConnect },
    { label: "断开连接", action: handleDisconnect },
    { divider: true, label: "" },
    { label: `目标: ${targetLabels[settingsStore.runtimeTarget]}`, disabled: true },
  ]

  switch (settingsStore.runtimeTarget) {
    case "stm32":
      items.push(
        { label: "ST-Link 下载", action: handleStlinkFlash },
        { label: "J-Link 下载", action: handleJlinkFlash },
      )
      break
    case "esp32":
      items.push(
        { label: "UART 下载固件", action: handleUartFlash },
      )
      break
    case "linux-arm":
    case "linux-x86":
      items.push(
        { label: "SSH 上传文件", action: handleSshTransfer },
      )
      break
  }

  items.push(
    { divider: true, label: "" },
    { label: "运行PLC", action: handleRunPlc },
    { label: "停止PLC", action: handleStopPlc },
    { label: "复位PLC", action: handleResetPlc },
  )

  return items
})

// ==================== 调试菜单 ====================
const debugMenuEntries: MenuItem[] = [
  { label: "启动调试", shortcut: "F5", action: handleStartDebug },
  { label: "停止调试", shortcut: "Shift+F5", action: handleStopDebug },
  { label: "暂停调试", action: handlePauseDebug },
  { label: "恢复运行", action: handleResumeDebug },
  { divider: true, label: "" },
  { label: "单步执行", shortcut: "F11", action: () => logAction("单步执行") },
  { label: "单步跳过", shortcut: "F10", action: () => logAction("单步跳过") },
  {
    label: "单步返回",
    shortcut: "Shift+F11",
    action: () => logAction("单步返回"),
  },
  { divider: true, label: "" },
  { label: "切换断点", shortcut: "F9", action: () => logAction("切换断点") },
  { label: "清除所有断点", action: () => logAction("清除所有断点") },
];

// ==================== 仿真器菜单 ====================
const simulatorMenuEntries: MenuItem[] = [
  { label: "设备仿真...", action: handleOpenCncSim },
  { label: "全场景仿真...", action: handleOpenWorldSim },
];

// ==================== 工具菜单 ====================
const toolsMenuEntries: MenuItem[] = [
  {
    label: "UART 调试",
    arrow: true,
    submenu: "uartDebug",
  },
  {
    label: "Modbus 调试",
    arrow: true,
    submenu: "modbusDebug",
  },
  {
    label: "CAN / CANopen 调试",
    arrow: true,
    submenu: "canDebug",
  },
  {
    label: "网络调试",
    arrow: true,
    submenu: "networkDebug",
  },
  {
    label: "电机调试",
    arrow: true,
    submenu: "motorDebug",
  },
  { divider: true, label: "" },
  {
    label: "代码格式化",
    shortcut: "Ctrl+Shift+F",
    action: () => logAction("代码格式化"),
  },
  { divider: true, label: "" },
  { label: "选项设置...", action: () => { settingsVisible.value = true; closeMenu() } },
];

// ==================== 窗口菜单 ====================
const windowMenuEntries: MenuItem[] = [
  { label: "关闭当前标签页", shortcut: "Ctrl+W", action: handleCloseActiveTab },
  { label: "关闭所有标签页", action: handleCloseAllTabs },
  { divider: true, label: "" },
  {
    label: "下一个标签页",
    shortcut: "Ctrl+Tab",
    action: () => logAction("下一个标签页"),
  },
  {
    label: "上一个标签页",
    shortcut: "Ctrl+Shift+Tab",
    action: () => logAction("上一个标签页"),
  },
];

// ==================== 帮助菜单 ====================
const helpMenuEntries: MenuItem[] = [
  { label: "关于 PLC Studio", action: () => logAction("关于 PLC Studio") },
  { label: "检查更新", action: () => logAction("检查更新") },
  { divider: true, label: "" },
  { label: "查看文档", action: () => logAction("查看文档") },
];

// ==================== 菜单操作处理 ====================
function executeAction(entry: MenuItem) {
  if (entry.disabled) return;
  entry.action?.();
  closeMenu();
}

function logAction(actionName: string) {
  debugStore.addLog("info", `[菜单] ${actionName}`);
  console.log(`[菜单操作] ${actionName}`);
}

// 文件操作
function handleNewProject() {
  router.push('/welcome')
}

function handleOpenProject() {
  fileInput.value?.click()
}

function onFileSelected(e: Event) {
  const target = e.target as HTMLInputElement
  const file = target.files?.[0]
  if (!file) return

  const reader = new FileReader()
  reader.onload = () => {
    try {
      const project = JSON.parse(reader.result as string) as PlcProject
      projectStore.openProject(file.name, project)
      debugStore.addLog("info", `已打开项目: ${project.name}`)
    } catch {
      debugStore.addLog("error", "无法解析项目文件")
    }
  }
  reader.readAsText(file)
  target.value = ""
}

function importFile(accept: string, label: string) {
  const input = document.createElement("input")
  input.type = "file"
  input.accept = accept
  input.onchange = (e) => {
    const file = (e.target as HTMLInputElement).files?.[0]
    if (!file) return
    const reader = new FileReader()
    reader.onload = () => {
      debugStore.addLog("info", `已导入 ${label}: ${file.name}`)
      // TODO: 解析并集成到当前项目
    }
    reader.readAsText(file)
  }
  input.click()
  closeMenu()
}

function handleSaveProject() {
  projectStore.saveCurrentProject();
  debugStore.addLog("info", "项目已保存");
}

function handleSaveProjectAs() {
  if (!projectStore.projectName) {
    debugStore.addLog("warning", "没有打开的项目")
    return
  }
  const project: PlcProject = {
    name: projectStore.projectName,
    path: projectStore.projectPath,
    category: projectStore.projectCategory as any,
    pous: projectStore.pous,
    dataTypes: projectStore.dataTypes,
    configurations: projectStore.configurations,
  }
  const blob = new Blob([JSON.stringify(project, null, 2)], { type: "application/json" })
  const url = URL.createObjectURL(blob)
  const a = document.createElement("a")
  a.href = url
  a.download = `${projectStore.projectName}.plcproj`
  a.click()
  URL.revokeObjectURL(url)
  debugStore.addLog("info", `项目已导出: ${projectStore.projectName}.plcproj`)
}

function handleExit() {
  if (projectStore.projectModified) {
    if (!confirm("项目已修改，是否保存？")) return;
    projectStore.saveCurrentProject();
  }
  window.close();
}

async function handleOpenSampleProject(dirName: string) {
  const samples: Record<string, { name: string; pous: any[] }> = {
    '01_basic_logic': {
      name: '位逻辑示例',
      pous: [{
        name: 'BasicLogic',
        pouType: 0,
        variables: { inputVars: [], outputVars: [], inOutVars: [], localVars: [
          { name: 'bStart', type: 'BOOL', className: 'Local', initialValue: false, comment: '启动' },
          { name: 'bStop', type: 'BOOL', className: 'Local', initialValue: false, comment: '停止' },
          { name: 'bRun', type: 'BOOL', className: 'Local', initialValue: false, comment: '运行中' },
        ], tempVars: [], globalVars: [], externalVars: [], accessVars: [] },
        body: 'PROGRAM BasicLogic\nbRun := bStart AND NOT bStop;\nEND_PROGRAM',
        bodyLanguage: 'ST',
        comment: '基本位逻辑示例：启停控制',
      }],
      dataTypes: [],
      configurations: [{ name: 'Config', resources: [{ name: 'Main', tasks: [{ name: 'Task1', interval: 'T#10ms', priority: 1 }], programs: [{ name: 'BasicLogic', typeName: 'BasicLogic', taskName: 'Task1' }] }] }],
    },
    '02_timer_counter': {
      name: '定时器/计数器示例',
      pous: [{
        name: 'TimerCounter',
        pouType: 0,
        variables: { inputVars: [], outputVars: [], inOutVars: [], localVars: [
          { name: 'nCount', type: 'INT', className: 'Local', initialValue: 0, comment: '计数' },
          { name: 'bTick', type: 'BOOL', className: 'Local', initialValue: false, comment: '脉冲' },
          { name: 'tOn', type: 'TIME', className: 'Local', initialValue: 0, comment: '计时' },
        ], tempVars: [], globalVars: [], externalVars: [], accessVars: [] },
        body: 'PROGRAM TimerCounter\nIF bTick THEN\n  nCount := nCount + 1;\nEND_IF\nEND_PROGRAM',
        bodyLanguage: 'ST',
        comment: '定时器与计数器示例',
      }],
      dataTypes: [],
      configurations: [{ name: 'Config', resources: [{ name: 'Main', tasks: [{ name: 'Task1', interval: 'T#10ms', priority: 1 }], programs: [{ name: 'TimerCounter', typeName: 'TimerCounter', taskName: 'Task1' }] }] }],
    },
    '03_analog_processing': {
      name: '模拟量处理示例',
      pous: [{
        name: 'AnalogProc',
        pouType: 0,
        variables: { inputVars: [], outputVars: [], inOutVars: [], localVars: [
          { name: 'fRaw', type: 'REAL', className: 'Local', initialValue: 0.0, comment: '原始值' },
          { name: 'fScaled', type: 'REAL', className: 'Local', initialValue: 0.0, comment: '标定值' },
          { name: 'fFiltered', type: 'REAL', className: 'Local', initialValue: 0.0, comment: '滤波值' },
        ], tempVars: [], globalVars: [], externalVars: [], accessVars: [] },
        body: 'PROGRAM AnalogProc\nfScaled := fRaw * 100.0 / 27648.0;\nfFiltered := fFiltered * 0.9 + fScaled * 0.1;\nEND_PROGRAM',
        bodyLanguage: 'ST',
        comment: '模拟量采集与标定示例',
      }],
      dataTypes: [],
      configurations: [{ name: 'Config', resources: [{ name: 'Main', tasks: [{ name: 'Task1', interval: 'T#10ms', priority: 1 }], programs: [{ name: 'AnalogProc', typeName: 'AnalogProc', taskName: 'Task1' }] }] }],
    },
    '04_state_machine': {
      name: '状态机示例',
      pous: [{
        name: 'StateMachine',
        pouType: 0,
        variables: { inputVars: [], outputVars: [], inOutVars: [], localVars: [
          { name: 'nState', type: 'INT', className: 'Local', initialValue: 0, comment: '当前状态' },
          { name: 'bInit', type: 'BOOL', className: 'Local', initialValue: false, comment: '初始化' },
          { name: 'bDone', type: 'BOOL', className: 'Local', initialValue: false, comment: '完成' },
        ], tempVars: [], globalVars: [], externalVars: [], accessVars: [] },
        body: 'PROGRAM StateMachine\nCASE nState OF\n  0: IF bInit THEN nState := 1; END_IF\n  1: nState := 2;\n  2: bDone := TRUE; nState := 0;\nEND_CASE\nEND_PROGRAM',
        bodyLanguage: 'ST',
        comment: '简单状态机示例',
      }],
      dataTypes: [],
      configurations: [{ name: 'Config', resources: [{ name: 'Main', tasks: [{ name: 'Task1', interval: 'T#10ms', priority: 1 }], programs: [{ name: 'StateMachine', typeName: 'StateMachine', taskName: 'Task1' }] }] }],
    },
    '05_motion_control': {
      name: '运动控制示例',
      pous: [{
        name: 'MotionCtrl',
        pouType: 0,
        variables: { inputVars: [], outputVars: [], inOutVars: [], localVars: [
          { name: 'fSpeed', type: 'REAL', className: 'Local', initialValue: 100.0, comment: '速度' },
          { name: 'fPosition', type: 'REAL', className: 'Local', initialValue: 0.0, comment: '位置' },
          { name: 'fAccel', type: 'REAL', className: 'Local', initialValue: 50.0, comment: '加速度' },
        ], tempVars: [], globalVars: [], externalVars: [], accessVars: [] },
        body: 'PROGRAM MotionCtrl\nfPosition := fPosition + fSpeed * 0.001;\nEND_PROGRAM',
        bodyLanguage: 'ST',
        comment: '运动控制示例：速度与位置',
      }],
      dataTypes: [],
      configurations: [{ name: 'Config', resources: [{ name: 'Main', tasks: [{ name: 'Task1', interval: 'T#10ms', priority: 1 }], programs: [{ name: 'MotionCtrl', typeName: 'MotionCtrl', taskName: 'Task1' }] }] }],
    },
  }

  const data = samples[dirName]
  if (data) {
    projectStore.openProject(`samples/${dirName}`, { ...data, path: `samples/${dirName}`, category: 'standard' } as any)
    debugStore.addLog("info", `已打开示例项目: ${data.name}`)
  } else {
    debugStore.addLog("error", `示例项目不存在: ${dirName}`)
  }
}

// 编辑操作
function handleUndo() {
  editorStore.undo();
}

function handleRedo() {
  editorStore.redo();
}

function handleCut() {
  editorStore.cutSelection();
}

function handleCopy() {
  editorStore.copySelection();
}

function handlePaste() {
  editorStore.pasteClipboard();
}

function handleSelectAll() {
  editorStore.selectAll();
}

function handleFind() {
  logAction("查找");
}

function handleReplace() {
  logAction("替换");
}

// 视图操作
function toggleView(key: string) {
  switch (key) {
    case "activityBar":
      uiStore.activityBarVisible = !uiStore.activityBarVisible;
      break;
    case "sidebar":
      uiStore.sidebarVisible = !uiStore.sidebarVisible;
      break;
    case "bottomPanel":
      uiStore.bottomPanelVisible = !uiStore.bottomPanelVisible;
      break;
    case "statusBar":
      uiStore.statusBarVisible = !uiStore.statusBarVisible;
      break;
  }
}

function resetLayout() {
  uiStore.activityBarVisible = true;
  uiStore.sidebarVisible = true;
  uiStore.bottomPanelVisible = true;
  uiStore.statusBarVisible = true;
  closeMenu();
}

// 在线操作
function handleConnect() {
  const target = settingsStore.runtimeTarget
  if (target === "stm32" || target === "esp32") {
    const uartCfg = settingsStore.uart
    const chipName = target === "stm32" ? "STM32" : "ESP32"
    debugStore.addLog("info", `通过 ${uartCfg.port} (${uartCfg.baudRate}) 连接 ${chipName}...`)
    debugStore.connect("127.0.0.1", 3000, { port: uartCfg.port, baudRate: uartCfg.baudRate })
  } else {
    const host = prompt("请输入PLC设备地址:", debugStore.host || "127.0.0.1");
    if (host) {
      const portStr = prompt("请输入端口号:", String(debugStore.port || 3000));
      if (portStr) {
        const port = parseInt(portStr, 10);
        if (!isNaN(port)) {
          debugStore.connect(host, port);
          debugStore.addLog("info", `已连接到设备: ${host}:${port}`);
        }
      }
    }
  }
}

function handleDisconnect() {
  debugStore.disconnect();
  debugStore.addLog("info", "已断开设备连接");
}

async function handleStlinkFlash() {
  logAction("ST-Link 下载")
  try {
    debugStore.addLog("info", "[1/2] 编译固件中...")
    const resp = await fetch('http://127.0.0.1:3000/api/build/flash/stlink', { method: 'POST' })
    const data = await resp.json()
    if (data.success) {
      debugStore.addLog("info", "下载成功！")
    } else {
      debugStore.addLog("error", `下载失败[${data.step}]: ${data.error || '未知错误'}`)
    }
  } catch {
    debugStore.addLog("error", "无法连接服务器，请确保后端已启动")
  }
}

async function handleJlinkFlash() {
  logAction("J-Link 下载")
  try {
    debugStore.addLog("info", "[1/2] 编译固件中...")
    const resp = await fetch('http://127.0.0.1:3000/api/build/flash/jlink', { method: 'POST' })
    const data = await resp.json()
    if (data.success) {
      debugStore.addLog("info", "下载成功！")
    } else {
      debugStore.addLog("error", `下载失败[${data.step}]: ${data.error || '未知错误'}`)
    }
  } catch {
    debugStore.addLog("error", "无法连接服务器，请确保后端已启动")
  }
}

async function handleUartFlash() {
  logAction("UART 下载固件")
  try {
    debugStore.addLog("info", "[1/2] 编译固件中...")
    const port = settingsStore.uart.port
    const resp = await fetch('http://127.0.0.1:3000/api/build/flash/esp32', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ port }),
    })
    const data = await resp.json()
    if (data.success) {
      debugStore.addLog("info", "下载成功！")
    } else {
      debugStore.addLog("error", `下载失败[${data.step}]: ${data.error || '未知错误'}`)
    }
  } catch {
    debugStore.addLog("error", "无法连接服务器，请确保后端已启动")
  }
}

async function handleSshTransfer() {
  logAction("SSH 上传文件")
  try {
    debugStore.addLog("info", "[1/2] 编译固件中...")
    const resp = await fetch('http://127.0.0.1:3000/api/build/flash/ssh', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ host: '192.168.1.100', port: 22 }),
    })
    const data = await resp.json()
    if (data.success) {
      debugStore.addLog("info", "上传成功！")
    } else {
      debugStore.addLog("error", `上传失败[${data.step}]: ${data.error || '未知错误'}`)
    }
  } catch {
    debugStore.addLog("error", "无法连接服务器，请确保后端已启动")
  }
}

function handleRunPlc() {
  debugStore.startDebug();
}

function handleStopPlc() {
  debugStore.stopDebug();
}

function handleResetPlc() {
  debugStore.addLog("info", "PLC 已复位");
}

// 调试操作
function handleStartDebug() {
  if (debugStore.runtimeStatus === "disconnected") {
    debugStore.addLog("warning", "请先连接设备再启动调试");
    return;
  }
  debugStore.startDebug();
}

function handleStopDebug() {
  debugStore.stopDebug();
}

function handlePauseDebug() {
  debugStore.pauseDebug();
}

function handleResumeDebug() {
  debugStore.resumeDebug();
}

// 仿真器操作
function handleOpenCncSim() {
  router.push({ path: "/simulator", query: { type: "cnc" } });
}

function handleOpenWorldSim() {
  router.push({ path: "/simulator", query: { type: "world" } });
}

// 窗口操作
function handleCloseActiveTab() {
  editorStore.closeActiveTab();
}

function handleCloseAllTabs() {
  editorStore.closeAllTabs();
}

// 工具面板操作
function openToolPanel(toolId: string) {
  const toolNames: Record<string, string> = {
    "uart-terminal": "UART 终端",
    "uart-waveform": "UART 波形查看器",
    "uart-logger": "UART 数据记录",
    "uart-config": "UART 配置",
    "modbus-rtu": "Modbus RTU 调试器",
    "modbus-tcp": "Modbus TCP 调试器",
    "modbus-monitor": "Modbus 数据监控",
    "modbus-chart": "Modbus 寄存器图表",
    "modbus-analyzer": "Modbus 协议分析",
    "can-monitor": "CAN 总线监控",
    "can-sender": "CAN 帧发送器",
    "canopen-nmt": "CANopen 网络管理",
    "canopen-sdo": "CANopen SDO 调试",
    "canopen-pdo": "CANopen PDO 监控",
    "can-chart": "CAN 报文图表",
    "tcp-server": "TCP 服务端",
    "tcp-client": "TCP 客户端",
    "udp-server": "UDP 服务端",
    "udp-client": "UDP 客户端",
    "websocket-debug": "WebSocket 调试",
    "http-tester": "HTTP 请求测试",
    "network-chart": "网络数据图表",
    "pid-tuner": "PID 调谐器",
    "motor-response": "电机响应曲线",
    "stepper-debug": "步进电机调试",
    "servo-debug": "伺服电机调试",
    "vfd-debug": "变频器调试",
    "motion-chart": "运动轨迹图表",
    "torque-monitor": "扭矩/速度监控",
  };
  const toolName = toolNames[toolId] || toolId;
  debugStore.addLog("info", `打开工具: ${toolName}`);
  editorStore.openToolTab(toolId, toolName);
  closeMenu();
}

// ==================== 菜单交互 ====================
function toggleMenu(id: string) {
  activeMenu.value = activeMenu.value === id ? null : id;
  activeSubmenu.value = null;
}

function hoverMenu(id: string) {
  if (activeMenu.value !== null) {
    activeMenu.value = id;
    activeSubmenu.value = null;
  }
}

function closeMenu() {
  activeMenu.value = null;
  activeSubmenu.value = null;
}

function handleGlobalClick() {
  closeMenu();
}

// 全局快捷键
function handleKeydown(e: KeyboardEvent) {
  const ctrl = e.ctrlKey || e.metaKey;
  const shift = e.shiftKey;

  if (ctrl && !shift) {
    switch (e.key.toLowerCase()) {
      case "n":
        e.preventDefault();
        handleNewProject();
        break;
      case "o":
        e.preventDefault();
        handleOpenProject();
        break;
      case "s":
        e.preventDefault();
        handleSaveProject();
        break;
      case "z":
        e.preventDefault();
        handleUndo();
        break;
      case "y":
        e.preventDefault();
        handleRedo();
        break;
      case "w":
        e.preventDefault();
        handleCloseActiveTab();
        break;
      case "f":
        e.preventDefault();
        handleFind();
        break;
      case "h":
        e.preventDefault();
        handleReplace();
        break;
      case "a":
        e.preventDefault();
        handleSelectAll();
        break;
    }
  } else if (ctrl && shift) {
    switch (e.key.toLowerCase()) {
      case "s":
        e.preventDefault();
        handleSaveProjectAs();
        break;
      case "f":
        e.preventDefault();
        logAction("代码格式化");
        break;
    }
  }
}

onMounted(() => {
  document.addEventListener("click", handleGlobalClick);
  document.addEventListener("keydown", handleKeydown);
});

onUnmounted(() => {
  document.removeEventListener("click", handleGlobalClick);
  document.removeEventListener("keydown", handleKeydown);
});
</script>

<style scoped>
.top-nav {
  height: var(--toolbar-height);
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: var(--surface-container-highest);
  border-bottom: 1px solid var(--outline-variant);
  padding: 0 var(--padding-md);
  user-select: none;
  z-index: 50;
  flex-shrink: 0;
}

.nav-left {
  display: flex;
  align-items: center;
  gap: var(--padding-md);
}

.app-brand {
  font-size: 18px;
  font-weight: 700;
  color: var(--secondary);
  font-family: "Inter", sans-serif;
}

.nav-menu {
  display: flex;
  align-items: center;
  height: 100%;
  gap: 2px;
}

.nav-menu-wrapper {
  position: relative;
  height: 100%;
}

.nav-menu-item {
  padding: 0 var(--padding-sm);
  height: 100%;
  display: flex;
  align-items: center;
  background: none;
  border: none;
  border-bottom: 2px solid transparent;
  font-family: "Inter", sans-serif;
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  color: var(--on-surface-variant);
  cursor: pointer;
  transition:
    background var(--transition-fast),
    color var(--transition-fast);
}

.nav-menu-item:hover {
  background: var(--surface-variant);
}

.nav-menu-item.active {
  color: var(--primary);
  border-bottom-color: var(--primary);
}

.nav-right {
  display: flex;
  align-items: center;
  gap: var(--padding-sm);
}

.search-box {
  display: flex;
  align-items: center;
  background: var(--surface-variant);
  border-radius: var(--radius);
  padding: 0 var(--padding-sm);
  gap: 4px;
  margin-right: var(--padding-sm);
}

.search-box .material-symbols-outlined {
  font-size: 16px;
  color: var(--on-surface-variant);
}

.search-box input {
  background: none;
  border: none;
  outline: none;
  color: var(--on-surface);
  font-size: 11px;
  font-family: "Inter", sans-serif;
  width: 160px;
  padding: 4px 0;
}

.search-box input::placeholder {
  color: var(--on-surface-variant);
}

.btn-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 28px;
  height: 28px;
  background: none;
  border: none;
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  cursor: pointer;
  transition: background var(--transition-fast);
}

.btn-icon:hover {
  background: var(--surface-variant);
}

.btn-run {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 28px;
  height: 28px;
  background: var(--secondary-container);
  border: none;
  border-radius: var(--radius);
  color: var(--on-secondary-container);
  cursor: pointer;
  transition: filter var(--transition-fast);
}

.btn-run:hover {
  filter: brightness(1.1);
}

/* 下拉菜单 */
.dropdown-menu {
  position: absolute;
  top: 100%;
  left: 0;
  min-width: 220px;
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius-lg);
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.4);
  padding: 4px;
  z-index: 100;
}

.dropdown-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 12px;
  border-radius: var(--radius);
  cursor: pointer;
  transition: background 0.15s;
  font-size: 12px;
  color: var(--on-surface);
  position: relative;
  white-space: nowrap;
}

.dropdown-item:hover {
  background: var(--surface-variant);
}

.dropdown-item.disabled {
  color: var(--on-surface-variant);
  opacity: 0.4;
  pointer-events: none;
}

.dropdown-check-space {
  width: 16px;
  height: 16px;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.dropdown-check-space .material-symbols-outlined {
  font-size: 16px;
}

.dropdown-label {
  flex: 1;
}

.dropdown-shortcut {
  color: var(--on-surface-variant);
  font-size: 11px;
  font-family: "JetBrains Mono", monospace;
  margin-left: 24px;
}

.dropdown-arrow {
  font-size: 16px;
  color: var(--on-surface-variant);
  flex-shrink: 0;
}

.dropdown-divider {
  height: 1px;
  background: var(--outline-variant);
  margin: 4px 0;
}

/* 子菜单 */
.dropdown-parent {
  position: relative;
}

.dropdown-submenu {
  position: absolute;
  left: 100%;
  top: -4px;
  min-width: 200px;
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius-lg);
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.4);
  padding: 4px;
  z-index: 101;
}
</style>
