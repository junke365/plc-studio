<template>
  <div class="tool-panel-container">
    <component :is="toolComponent" v-if="toolComponent" />
    <div v-else class="tool-not-found">
      <span class="material-symbols-outlined">build</span>
      <p>工具面板未找到: {{ toolId }}</p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, defineAsyncComponent } from 'vue';

const props = defineProps<{ toolId: string }>();

// 工具组件映射表
const toolMap: Record<string, ReturnType<typeof defineAsyncComponent>> = {
  // UART 工具
  'uart-terminal': defineAsyncComponent(() => import('../components/tools/uart/UartTerminal.vue')),
  'uart-waveform': defineAsyncComponent(() => import('../components/tools/uart/UartWaveform.vue')),
  'uart-logger': defineAsyncComponent(() => import('../components/tools/uart/UartLogger.vue')),
  'uart-config': defineAsyncComponent(() => import('../components/tools/uart/UartConfig.vue')),
  // Modbus 工具
  'modbus-rtu': defineAsyncComponent(() => import('../components/tools/modbus/ModbusRtu.vue')),
  'modbus-tcp': defineAsyncComponent(() => import('../components/tools/modbus/ModbusTcp.vue')),
  'modbus-monitor': defineAsyncComponent(() => import('../components/tools/modbus/ModbusMonitor.vue')),
  'modbus-chart': defineAsyncComponent(() => import('../components/tools/modbus/ModbusChart.vue')),
  'modbus-analyzer': defineAsyncComponent(() => import('../components/tools/modbus/ModbusAnalyzer.vue')),
  // CAN/CANopen 工具
  'can-monitor': defineAsyncComponent(() => import('../components/tools/can/CanMonitor.vue')),
  'can-sender': defineAsyncComponent(() => import('../components/tools/can/CanSender.vue')),
  'canopen-nmt': defineAsyncComponent(() => import('../components/tools/can/CanopenNmt.vue')),
  'canopen-sdo': defineAsyncComponent(() => import('../components/tools/can/CanopenSdo.vue')),
  'canopen-pdo': defineAsyncComponent(() => import('../components/tools/can/CanopenPdo.vue')),
  'can-chart': defineAsyncComponent(() => import('../components/tools/can/CanChart.vue')),
  // 网络调试工具
  'tcp-server': defineAsyncComponent(() => import('../components/tools/network/TcpServer.vue')),
  'tcp-client': defineAsyncComponent(() => import('../components/tools/network/TcpClient.vue')),
  'udp-server': defineAsyncComponent(() => import('../components/tools/network/UdpServer.vue')),
  'udp-client': defineAsyncComponent(() => import('../components/tools/network/UdpClient.vue')),
  'websocket-debug': defineAsyncComponent(() => import('../components/tools/network/WebSocketDebug.vue')),
  'http-tester': defineAsyncComponent(() => import('../components/tools/network/HttpTester.vue')),
  'network-chart': defineAsyncComponent(() => import('../components/tools/network/NetworkChart.vue')),
  // 电机调试工具
  'pid-tuner': defineAsyncComponent(() => import('../components/tools/motor/PidTuner.vue')),
  'motor-response': defineAsyncComponent(() => import('../components/tools/motor/MotorResponse.vue')),
  'stepper-debug': defineAsyncComponent(() => import('../components/tools/motor/StepperDebug.vue')),
  'servo-debug': defineAsyncComponent(() => import('../components/tools/motor/ServoDebug.vue')),
  'vfd-debug': defineAsyncComponent(() => import('../components/tools/motor/VfdDebug.vue')),
  'motion-chart': defineAsyncComponent(() => import('../components/tools/motor/MotionChart.vue')),
  'torque-monitor': defineAsyncComponent(() => import('../components/tools/motor/TorqueMonitor.vue')),
  // ROS 2 调试工具
  'ros2-debug': defineAsyncComponent(() => import('../components/tools/ros2/Ros2Debug.vue')),
};

const toolComponent = computed(() => toolMap[props.toolId] || null);
</script>

<style scoped>
.tool-panel-container {
  width: 100%;
  height: 100%;
  overflow: hidden;
}
.tool-not-found {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 100%;
  color: var(--on-surface-variant);
  gap: 8px;
}
.tool-not-found .material-symbols-outlined {
  font-size: 48px;
  opacity: 0.3;
}
</style>
