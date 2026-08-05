import { defineStore } from "pinia"
import { ref, reactive, watch } from "vue"

export type DebugMode = "uart" | "tcp" | "udp"
export type RuntimeTarget = "stm32" | "esp32" | "linux-arm" | "linux-x86"

export interface UartSettings {
  port: string
  baudRate: number
  dataBits: number
  stopBits: number
  parity: "none" | "odd" | "even" | "mark" | "space"
}

export interface TcpSettings {
  host: string
  port: number
  timeout: number
}

export interface UdpSettings {
  localPort: number
  remoteHost: string
  remotePort: number
  broadcast: boolean
}

const STORAGE_KEY = "plc-studio-settings"

function loadFromStorage<T>(key: string, fallback: T): T {
  try {
    const raw = localStorage.getItem(`${STORAGE_KEY}:${key}`)
    return raw ? JSON.parse(raw) : fallback
  } catch {
    return fallback
  }
}

function saveToStorage(key: string, value: unknown) {
  try {
    localStorage.setItem(`${STORAGE_KEY}:${key}`, JSON.stringify(value))
  } catch { /* ignore */ }
}

export const useSettingsStore = defineStore("settings", () => {
  const debugMode = ref<DebugMode>(
    loadFromStorage<DebugMode>("debugMode", "uart")
  )

  const runtimeTarget = ref<RuntimeTarget>(
    loadFromStorage<RuntimeTarget>("runtimeTarget", "stm32")
  )

  const uart = reactive<UartSettings>(
    loadFromStorage<UartSettings>("uart", {
      port: "",
      baudRate: 115200,
      dataBits: 8,
      stopBits: 1,
      parity: "none",
    })
  )

  const tcp = reactive<TcpSettings>(
    loadFromStorage<TcpSettings>("tcp", {
      host: "127.0.0.1",
      port: 502,
      timeout: 3000,
    })
  )

  const udp = reactive<UdpSettings>(
    loadFromStorage<UdpSettings>("udp", {
      localPort: 0,
      remoteHost: "127.0.0.1",
      remotePort: 502,
      broadcast: false,
    })
  )

  // 持久化到 localStorage
  watch(debugMode, (v) => saveToStorage("debugMode", v))
  watch(runtimeTarget, (v) => saveToStorage("runtimeTarget", v))
  watch(uart, (v) => saveToStorage("uart", v), { deep: true })
  watch(tcp, (v) => saveToStorage("tcp", v), { deep: true })
  watch(udp, (v) => saveToStorage("udp", v), { deep: true })

  function setDebugMode(mode: DebugMode) {
    debugMode.value = mode
  }

  function setRuntimeTarget(target: RuntimeTarget) {
    runtimeTarget.value = target
  }

  function resetToDefaults() {
    debugMode.value = "uart"
    runtimeTarget.value = "stm32"
    Object.assign(uart, {
      port: "",
      baudRate: 115200,
      dataBits: 8,
      stopBits: 1,
      parity: "none",
    })
    Object.assign(tcp, { host: "127.0.0.1", port: 502, timeout: 3000 })
    Object.assign(udp, { localPort: 0, remoteHost: "127.0.0.1", remotePort: 502, broadcast: false })
  }

  return {
    debugMode,
    runtimeTarget,
    uart,
    tcp,
    udp,
    setDebugMode,
    setRuntimeTarget,
    resetToDefaults,
  }
})
