<template>
  <div class="output-panel">
    <div class="output-scroll" ref="contentRef">
      <div
        v-for="(log, index) in debugStore.logs"
        :key="index"
        class="log-line"
        :class="log.level"
      >
        <span class="log-time">[{{ formatTime(log.timestamp) }}]</span>
        <span class="log-message">{{ log.message }}</span>
      </div>
      <div v-if="debugStore.logs.length === 0" class="no-output">暂无输出</div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from "vue";
import { useDebugStore } from "../../stores/debug";

const debugStore = useDebugStore();
const contentRef = ref<HTMLDivElement | null>(null);

function formatTime(date: Date): string {
  return date.toLocaleTimeString();
}
</script>

<style scoped>
.output-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.output-scroll {
  flex: 1;
  overflow-y: auto;
  padding: var(--padding-xs) var(--padding-sm);
}

.log-line {
  display: flex;
  gap: var(--padding-sm);
  padding: 1px 0;
  font-family: "JetBrains Mono", monospace;
  font-size: 11px;
  line-height: 16px;
}

.log-time {
  color: var(--outline);
  flex-shrink: 0;
}

.log-line.info {
  color: var(--on-surface);
}
.log-line.success {
  color: var(--secondary);
}
.log-line.error {
  color: var(--error);
}
.log-line.warning {
  color: #ffb74d;
}
.log-line.debug {
  color: var(--on-surface-variant);
}

.no-output {
  padding: var(--padding-md);
  text-align: center;
  font-size: 11px;
  color: var(--on-surface-variant);
}
</style>
