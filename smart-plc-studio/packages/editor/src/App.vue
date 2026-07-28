<template>
  <router-view />
</template>

<script setup lang="ts"></script>

<style>
:root {
  --surface: #0b1326;
  --surface-dim: #0b1326;
  --surface-bright: #31394d;
  --surface-container-lowest: #060e20;
  --surface-container-low: #131b2e;
  --surface-container: #171f33;
  --surface-container-high: #222a3d;
  --surface-container-highest: #2d3449;
  --surface-variant: #2d3449;
  --primary: #adc6ff;
  --on-primary: #002e6a;
  --primary-container: #4d8eff;
  --on-primary-container: #00285d;
  --secondary: #4edea3;
  --on-secondary: #003824;
  --secondary-container: #00a572;
  --on-secondary-container: #00311f;
  --tertiary: #fbabff;
  --on-tertiary: #580065;
  --tertiary-container: #e14ef6;
  --on-tertiary-container: #4d0059;
  --error: #ffb4ab;
  --on-error: #690005;
  --error-container: #93000a;
  --on-error-container: #ffdad6;
  --on-surface: #dae2fd;
  --on-surface-variant: #c2c6d6;
  --on-background: #dae2fd;
  --inverse-surface: #dae2fd;
  --inverse-on-surface: #283044;
  --inverse-primary: #005ac2;
  --outline: #8c909f;
  --outline-variant: #424754;
  --syntax-keyword: #e14ef6;
  --syntax-type: #4d8eff;
  --syntax-comment: #8c909f;
  --editor-bg: #0f172a;
  --sidebar-width: 260px;
  --inspector-width: 300px;
  --toolbar-height: 40px;
  --statusbar-height: 24px;
  --gutter: 1px;
  --padding-xs: 4px;
  --padding-sm: 8px;
  --padding-md: 12px;
  --radius-sm: 0.125rem;
  --radius: 0.25rem;
  --radius-md: 0.375rem;
  --radius-lg: 0.5rem;
  --radius-xl: 0.75rem;
  --radius-full: 9999px;
}

*,
*::before,
*::after {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

html,
body,
#app {
  width: 100%;
  height: 100%;
  overflow: hidden;
  font-family: "Inter", "Microsoft YaHei", sans-serif;
  font-size: 13px;
  line-height: 18px;
  background: var(--surface);
  color: var(--on-surface);
  -webkit-font-smoothing: antialiased;
}

.material-symbols-outlined {
  font-variation-settings:
    "FILL" 0,
    "wght" 400,
    "GRAD" 0,
    "opsz" 24;
  font-size: 18px;
  line-height: 1;
  vertical-align: middle;
}

::-webkit-scrollbar {
  width: 6px;
  height: 6px;
}
::-webkit-scrollbar-track {
  background: transparent;
}
::-webkit-scrollbar-thumb {
  background: var(--outline-variant);
  border-radius: 3px;
}
::-webkit-scrollbar-thumb:hover {
  background: var(--outline);
}

::selection {
  background: var(--primary-container);
  color: var(--on-primary-container);
}

@keyframes pulse-anim {
  0%,
  100% {
    opacity: 1;
  }
  50% {
    opacity: 0.5;
  }
}
</style>
