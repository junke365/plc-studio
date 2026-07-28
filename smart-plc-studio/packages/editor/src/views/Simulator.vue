<template>
  <div class="simulator-layout">
    <header class="sim-header">
      <button class="back-btn" @click="goBack">
        <span class="material-symbols-outlined">arrow_back</span>
        返回IDE
      </button>
      <div class="sim-tabs">
        <button
          class="sim-tab"
          :class="{ active: simType === 'cnc' }"
          @click="simType = 'cnc'"
        >
          <span class="material-symbols-outlined">precision_manufacturing</span>
          设备仿真
        </button>
        <button
          class="sim-tab"
          :class="{ active: simType === 'world' }"
          @click="simType = 'world'"
        >
          <span class="material-symbols-outlined">public</span>
          全场景仿真
        </button>
      </div>
      <div class="sim-status">
        <span class="status-dot" :class="connected ? 'connected' : ''" />
        {{ connected ? "已连接" : "未连接" }}
      </div>
    </header>
    <div class="sim-content">
      <CncSimPanel
        v-if="simType === 'cnc'"
        @connect="connected = true"
        @disconnect="connected = false"
      />
      <WorldSimPanel
        v-else-if="simType === 'world'"
        @connect="connected = true"
        @disconnect="connected = false"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from "vue";
import { useRouter, useRoute } from "vue-router";
import CncSimPanel from "../components/simulator/CncSimPanel.vue";
import WorldSimPanel from "../components/simulator/WorldSimPanel.vue";

const router = useRouter();
const route = useRoute();

const simType = ref<string>((route.query.type as string) || "cnc");
const connected = ref(false);

function goBack() {
  router.push("/");
}

onMounted(() => {
  if (route.query.type) {
    simType.value = route.query.type as string;
  }
});
</script>

<style scoped>
.simulator-layout {
  width: 100%;
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--surface);
  color: var(--on-surface);
}

.sim-header {
  height: 48px;
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 0 16px;
  background: var(--surface-container-highest);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
}

.back-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 4px 12px;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 12px;
  font-family: "Inter", sans-serif;
  cursor: pointer;
  transition: background 0.15s;
}

.back-btn:hover {
  background: var(--surface-container-high);
}

.back-btn .material-symbols-outlined {
  font-size: 16px;
}

.sim-tabs {
  display: flex;
  gap: 2px;
}

.sim-tab {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 16px;
  background: none;
  border: none;
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  font-size: 12px;
  font-weight: 600;
  font-family: "Inter", sans-serif;
  cursor: pointer;
  transition: all 0.15s;
}

.sim-tab:hover {
  background: var(--surface-variant);
}

.sim-tab.active {
  background: var(--primary-container);
  color: var(--on-primary-container);
}

.sim-tab .material-symbols-outlined {
  font-size: 16px;
}

.sim-status {
  margin-left: auto;
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 11px;
  color: var(--on-surface-variant);
}

.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: var(--error);
}

.status-dot.connected {
  background: var(--secondary);
  animation: pulse-anim 2s ease-in-out infinite;
}

.sim-content {
  flex: 1;
  overflow: hidden;
}
</style>
