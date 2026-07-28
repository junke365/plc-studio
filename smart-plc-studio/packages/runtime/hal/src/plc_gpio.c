/**
 * plc_gpio.c - GPIO 硬件抽象层实现
 *
 * 使用静态端口状态数组管理引脚电平
 * Linux 平台通过 sysfs 读写 GPIO
 * 支持中断回调表
 */

#include "plc_gpio.h"
#include <string.h>

#ifdef PLATFORM_LINUX_ARM
  #include <stdio.h>
  #include <fcntl.h>
  #include <unistd.h>
#endif

/* ========== 内部数据结构 ========== */

/** 单个端口的引脚状态 */
typedef struct {
  uint32_t            pin_state;      /* 当前输出电平位掩码 */
  uint32_t            pin_direction;  /* 方向位掩码: 0=输入, 1=输出 */
  uint8_t             pin_count;      /* 该端口引脚数 */
} PlcGpioPort;

/** 引脚中断回调条目 */
typedef struct {
  PlcGpioEdge         edge;
  PlcGpioIsrCallback  callback;
} PlcGpioIsrEntry;

/* ========== 静态变量 ========== */

static PlcGpioPort    s_ports[PLC_GPIO_MAX_PORTS];
static PlcGpioIsrEntry s_isr_table[PLC_GPIO_MAX_PINS];
static uint8_t        s_isr_port_map[PLC_GPIO_MAX_PINS];
static uint8_t        s_isr_pin_map[PLC_GPIO_MAX_PINS];
static uint8_t        s_isr_count = 0;
static uint8_t        s_init_done = 0;

/* ========== 内部辅助函数 ========== */

/** 检查端口和引脚是否合法 */
static int gpio_validate(uint8_t port, uint8_t pin) {
  if (port >= PLC_GPIO_MAX_PORTS || pin >= s_ports[port].pin_count) {
    return -1;
  }
  return 0;
}

/** 在 Linux 平台读取 sysfs GPIO 值 */
static int gpio_sysfs_read(uint8_t port, uint8_t pin, bool* value) {
#ifdef PLATFORM_LINUX_ARM
  char path[64];
  char val;
  int fd;
  /* 用 port * 32 + pin 计算全局 GPIO 编号 */
  int gpio_num = port * 32 + pin;
  snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio_num);
  fd = open(path, O_RDONLY);
  if (fd < 0) {
    return -2;
  }
  if (read(fd, &val, 1) != 1) {
    close(fd);
    return -3;
  }
  close(fd);
  *value = (val == '1');
  return 0;
#else
  (void)port;
  (void)pin;
  (void)value;
  return 0;
#endif
}

/** 在 Linux 平台写入 sysfs GPIO 值 */
static int gpio_sysfs_write(uint8_t port, uint8_t pin, bool value) {
#ifdef PLATFORM_LINUX_ARM
  char path[64];
  int fd;
  char val = value ? '1' : '0';
  int gpio_num = port * 32 + pin;
  snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio_num);
  fd = open(path, O_WRONLY);
  if (fd < 0) {
    return -2;
  }
  if (write(fd, &val, 1) != 1) {
    close(fd);
    return -3;
  }
  close(fd);
  return 0;
#else
  (void)port;
  (void)pin;
  (void)value;
  return 0;
#endif
}

/* ========== 公共接口实现 ========== */

int plc_gpio_init(void) {
  memset(s_ports, 0, sizeof(s_ports));
  memset(s_isr_table, 0, sizeof(s_isr_table));
  memset(s_isr_port_map, 0, sizeof(s_isr_port_map));
  memset(s_isr_pin_map, 0, sizeof(s_isr_pin_map));
  s_isr_count = 0;

  /* 设置每个端口的默认引脚数 */
  for (uint8_t p = 0; p < PLC_GPIO_MAX_PORTS; p++) {
#ifdef PLATFORM_STM32
    s_ports[p].pin_count = 16;
#else
    s_ports[p].pin_count = 32;
#endif
  }

  s_init_done = 1;
  return 0;
}

int plc_gpio_set_mode(uint8_t port, uint8_t pin, PlcGpioMode mode) {
  if (gpio_validate(port, pin) != 0) {
    return -1;
  }
  /* 更新方向位掩码 */
  if (mode == PLC_GPIO_MODE_INPUT ||
      mode == PLC_GPIO_MODE_INPUT_PULLUP ||
      mode == PLC_GPIO_MODE_INPUT_PULLDOWN) {
    s_ports[port].pin_direction &= ~(1u << pin);
  } else {
    s_ports[port].pin_direction |= (1u << pin);
  }
  return 0;
}

int plc_gpio_write(uint8_t port, uint8_t pin, bool value) {
  if (gpio_validate(port, pin) != 0) {
    return -1;
  }
  /* 更新静态状态 */
  if (value) {
    s_ports[port].pin_state |= (1u << pin);
  } else {
    s_ports[port].pin_state &= ~(1u << pin);
  }
  /* 尝试写入硬件 */
  gpio_sysfs_write(port, pin, value);
  return 0;
}

int plc_gpio_read(uint8_t port, uint8_t pin, bool* value) {
  if (gpio_validate(port, pin) != 0 || value == NULL) {
    return -1;
  }
  /* 先尝试从硬件读取 */
  int ret = gpio_sysfs_read(port, pin, value);
  if (ret == 0) {
    /* 更新静态状态 */
    if (*value) {
      s_ports[port].pin_state |= (1u << pin);
    } else {
      s_ports[port].pin_state &= ~(1u << pin);
    }
  } else {
    /* 回退到静态状态 */
    *value = (s_ports[port].pin_state >> pin) & 1u;
  }
  return 0;
}

int plc_gpio_toggle(uint8_t port, uint8_t pin) {
  if (gpio_validate(port, pin) != 0) {
    return -1;
  }
  s_ports[port].pin_state ^= (1u << pin);
  bool val = (s_ports[port].pin_state >> pin) & 1u;
  gpio_sysfs_write(port, pin, val);
  return 0;
}

int plc_gpio_read_port(uint8_t port, uint32_t* values) {
  if (port >= PLC_GPIO_MAX_PORTS || values == NULL) {
    return -1;
  }
  *values = s_ports[port].pin_state;
  return 0;
}

int plc_gpio_write_port(uint8_t port, uint32_t mask, uint32_t values) {
  if (port >= PLC_GPIO_MAX_PORTS) {
    return -1;
  }
  /* 只修改 mask 为 1 的位 */
  s_ports[port].pin_state = (s_ports[port].pin_state & ~mask) |
                            (values & mask);
  return 0;
}

int plc_gpio_set_isr_callback(uint8_t port, uint8_t pin,
                               PlcGpioEdge edge,
                               PlcGpioIsrCallback callback) {
  if (gpio_validate(port, pin) != 0) {
    return -1;
  }

  /* 查找是否已注册 */
  for (uint8_t i = 0; i < s_isr_count; i++) {
    if (s_isr_port_map[i] == port && s_isr_pin_map[i] == pin) {
      if (callback == NULL) {
        /* 删除回调: 用最后一个覆盖 */
        s_isr_table[i] = s_isr_table[s_isr_count - 1];
        s_isr_port_map[i] = s_isr_port_map[s_isr_count - 1];
        s_isr_pin_map[i] = s_isr_pin_map[s_isr_count - 1];
        s_isr_count--;
      } else {
        s_isr_table[i].edge = edge;
        s_isr_table[i].callback = callback;
      }
      return 0;
    }
  }

  /* 新增回调 */
  if (callback != NULL && s_isr_count < PLC_GPIO_MAX_PINS) {
    s_isr_table[s_isr_count].edge = edge;
    s_isr_table[s_isr_count].callback = callback;
    s_isr_port_map[s_isr_count] = port;
    s_isr_pin_map[s_isr_count] = pin;
    s_isr_count++;
  }
  return 0;
}

uint8_t plc_gpio_get_port_count(void) {
  return PLC_GPIO_MAX_PORTS;
}

uint8_t plc_gpio_get_pin_count(uint8_t port) {
  if (port >= PLC_GPIO_MAX_PORTS) {
    return 0;
  }
  return s_ports[port].pin_count;
}

int plc_gpio_get_status(PlcGpioStatus* status) {
  if (status == NULL) {
    return -1;
  }
  status->port_count = PLC_GPIO_MAX_PORTS;
  status->configured_pins = 0;
  for (uint8_t p = 0; p < PLC_GPIO_MAX_PORTS; p++) {
    status->port_pins[p] = s_ports[p].pin_count;
    /* 统计已配置的引脚数 (方向位为 1 的) */
    uint32_t dir = s_ports[p].pin_direction;
    while (dir) {
      if (dir & 1u) status->configured_pins++;
      dir >>= 1;
    }
  }
  return 0;
}
