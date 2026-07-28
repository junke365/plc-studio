<template>
  <div class="debug-panel">
    <div class="panel-header">
      <span class="panel-title">调试</span>
      <button
        class="btn-run-debug"
        v-if="debugStore.debugStatus === 'disconnected'"
        title="启动调试"
        @click="debugStore.startDebug()"
      >
        <span
          class="material-symbols-outlined"
          style="font-variation-settings: &quot;FILL&quot; 1"
          >play_arrow</span
        >
      </button>
      <button
        class="btn-stop-debug"
        v-else
        title="停止调试"
        @click="debugStore.stopDebug()"
      >
        <span class="material-symbols-outlined">stop</span>
      </button>
    </div>

    <div class="debug-scroll">
      <!-- 连接 -->
      <div class="debug-section">
        <div class="section-title">连接</div>
        <div class="connection-form">
          <input class="input" v-model="debugStore.host" placeholder="主机" />
          <input
            class="input input-sm"
            v-model.number="debugStore.port"
            placeholder="端口"
            type="number"
          />
          <button class="btn-connect" @click="connect">连接</button>
        </div>
      </div>

      <!-- 变量监控 -->
      <div class="debug-section">
        <div class="section-title">变量监控</div>
        <div class="watch-list">
          <div
            v-for="variable in debugStore.watchVariables"
            :key="variable.path"
            class="watch-item"
          >
            <span class="watch-path">{{ variable.path }}</span>
            <span class="watch-value">{{ variable.value }}</span>
            <span
              class="material-symbols-outlined watch-remove"
              @click="debugStore.removeWatchVariable(variable.path)"
              >close</span
            >
          </div>
          <div v-if="debugStore.watchVariables.length === 0" class="no-data">
            暂无监控变量
          </div>
        </div>
      </div>

      <!-- 断点 -->
      <div class="debug-section">
        <div class="section-title">断点</div>
        <div class="breakpoint-list">
          <div
            v-for="breakpoint in debugStore.breakpoints"
            :key="breakpoint.id"
            class="bp-item"
          >
            <input
              type="checkbox"
              :checked="breakpoint.enabled"
              @change="debugStore.toggleBreakpoint(breakpoint.id)"
            />
            <span class="bp-file"
              >{{ breakpoint.path }}:{{ breakpoint.line }}</span
            >
            <span
              class="material-symbols-outlined watch-remove"
              @click="debugStore.removeBreakpoint(breakpoint.id)"
              >close</span
            >
          </div>
          <div v-if="debugStore.breakpoints.length === 0" class="no-data">
            暂无断点
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useDebugStore } from "../../stores/debug";

const debugStore = useDebugStore();

function connect() {
  debugStore.connect(debugStore.host, debugStore.port);
}
</script>

<style scoped>
.debug-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.panel-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: var(--padding-sm);
  background: var(--surface-container-high);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
}

.panel-title {
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  color: var(--on-surface);
}

.btn-run-debug,
.btn-stop-debug {
  width: 24px;
  height: 24px;
  display: flex;
  align-items: center;
  justify-content: center;
  border: none;
  border-radius: var(--radius);
  cursor: pointer;
}

.btn-run-debug {
  background: var(--secondary-container);
  color: var(--on-secondary-container);
}

.btn-stop-debug {
  background: var(--error-container);
  color: var(--error);
}

.debug-scroll {
  flex: 1;
  overflow-y: auto;
}

.debug-section {
  border-bottom: 1px solid var(--outline-variant);
}

.section-title {
  padding: 4px var(--padding-sm);
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  color: var(--outline);
}

.connection-form {
  display: flex;
  gap: 4px;
  padding: var(--padding-sm);
}

.connection-form .input {
  flex: 1;
  font-size: 11px;
}

.connection-form .input-sm {
  width: 60px;
  flex: none;
}

.btn-connect {
  padding: 0 var(--padding-sm);
  background: var(--primary-container);
  color: var(--on-primary-container);
  border: none;
  border-radius: var(--radius);
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  cursor: pointer;
}

.watch-list,
.breakpoint-list {
  max-height: 200px;
  overflow-y: auto;
}

.watch-item,
.bp-item {
  display: flex;
  align-items: center;
  gap: var(--padding-sm);
  padding: 2px var(--padding-sm);
  font-size: 11px;
}

.watch-item:hover,
.bp-item:hover {
  background: var(--surface-variant);
}

.watch-path,
.bp-file {
  flex: 1;
  color: var(--on-surface);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  font-family: "JetBrains Mono", monospace;
  font-size: 11px;
}

.watch-value {
  color: var(--primary);
  font-family: "JetBrains Mono", monospace;
  font-size: 11px;
}

.watch-remove {
  font-size: 14px;
  cursor: pointer;
  opacity: 0;
  transition:
    opacity var(--transition-fast),
    color var(--transition-fast);
  color: var(--on-surface-variant);
}

.watch-item:hover .watch-remove,
.bp-item:hover .watch-remove {
  opacity: 1;
}

.watch-remove:hover {
  color: var(--error);
}

.no-data {
  padding: var(--padding-md);
  text-align: center;
  font-size: 11px;
  color: var(--on-surface-variant);
}
</style>
