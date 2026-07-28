<template>
  <div class="debug-output">
    <div class="output-scroll">
      <div class="no-output">调试输出区域</div>
    </div>
  </div>
</template>

<script setup lang="ts"></script>

<style scoped>
.debug-output {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.output-scroll {
  flex: 1;
  overflow-y: auto;
  padding: var(--padding-md);
}

.no-output {
  text-align: center;
  font-size: 11px;
  color: var(--on-surface-variant);
}
</style>
