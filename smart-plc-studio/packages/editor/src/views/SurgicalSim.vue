<template>
  <div class="surgical-sim">
    <header class="sim-header">
      <button class="back-btn" @click="router.push('/')">
        <span class="material-symbols-outlined">arrow_back</span>
        返回 IDE
      </button>
      <div class="sim-title">
        <span class="material-symbols-outlined">biotech</span>
        手术机器人仿真器
        <span class="sim-subtitle">da Vinci 运动学 + 物理引擎</span>
      </div>
      <div class="connection-status" :class="{ connected: connected }">
        <span class="status-dot"></span>
        {{ connected ? "运行时已连接" : "离线模式" }}
      </div>
    </header>
    <div class="sim-body">
      <SurgicalSimPanel @connect="connected = true" @disconnect="connected = false" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from "vue";
import { useRouter } from "vue-router";
import SurgicalSimPanel from "../components/simulator/SurgicalSimPanel.vue";

const router = useRouter();
const connected = ref(false);
</script>

<style scoped>
.surgical-sim {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
  background: var(--background);
  color: var(--on-background);
}

.sim-header {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 8px 16px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  z-index: 10;
}

.back-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 6px 12px;
  background: none;
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  cursor: pointer;
  font-size: 13px;
  transition: all var(--transition-fast);
}

.back-btn:hover {
  background: var(--surface-variant);
  color: var(--on-surface);
}

.sim-title {
  flex: 1;
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 16px;
  font-weight: 600;
}

.sim-title .material-symbols-outlined {
  font-size: 24px;
  color: var(--tertiary);
}

.sim-subtitle {
  font-size: 12px;
  font-weight: 400;
  color: var(--on-surface-variant);
  margin-left: 8px;
}

.connection-status {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 12px;
  color: var(--on-surface-variant);
}

.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: var(--error);
  transition: all var(--transition-fast);
}

.connection-status.connected .status-dot {
  background: var(--primary);
  box-shadow: 0 0 6px var(--primary);
  animation: pulse 2s infinite;
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.5; }
}

.sim-body {
  flex: 1;
  overflow: hidden;
}
</style>
