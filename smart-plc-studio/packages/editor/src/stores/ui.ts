import { defineStore } from 'pinia'
import { ref } from 'vue'

export const useUiStore = defineStore('ui', () => {
  // 主题
  const isDark = ref<boolean>(true)

  // 侧边栏
  const sidebarVisible = ref<boolean>(true)
  const sidebarWidth = ref<number>(260)
  const sidebarTab = ref<string>('project')

  // 底部面板
  const bottomPanelVisible = ref<boolean>(true)
  const bottomPanelHeight = ref<number>(200)
  const bottomPanelTab = ref<string>('output')

  // 右侧面板
  const rightPanelVisible = ref<boolean>(false)
  const rightPanelWidth = ref<number>(300)

  // 活动栏
  const activityBarVisible = ref<boolean>(true)

  // 状态栏
  const statusBarVisible = ref<boolean>(true)

  // 缩放
  const zoom = ref<number>(1)

  // 切换主题
  function toggleTheme() {
    isDark.value = !isDark.value
  }

  // 切换侧边栏
  function toggleSidebar() {
    sidebarVisible.value = !sidebarVisible.value
  }

  // 切换底部面板
  function toggleBottomPanel() {
    bottomPanelVisible.value = !bottomPanelVisible.value
  }

  // 切换右侧面板
  function toggleRightPanel() {
    rightPanelVisible.value = !rightPanelVisible.value
  }

  // 设置侧边栏宽度
  function setSidebarWidth(width: number) {
    sidebarWidth.value = Math.max(180, Math.min(500, width))
  }

  // 设置底部面板高度
  function setBottomPanelHeight(height: number) {
    bottomPanelHeight.value = Math.max(100, Math.min(400, height))
  }

  return {
    isDark,
    sidebarVisible,
    sidebarWidth,
    sidebarTab,
    bottomPanelVisible,
    bottomPanelHeight,
    bottomPanelTab,
    rightPanelVisible,
    rightPanelWidth,
    activityBarVisible,
    statusBarVisible,
    zoom,
    toggleTheme,
    toggleSidebar,
    toggleBottomPanel,
    toggleRightPanel,
    setSidebarWidth,
    setBottomPanelHeight
  }
})
