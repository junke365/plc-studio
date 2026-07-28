/**
 * linux-x86/platform.c - x86 Linux 平台适配
 *
 * 基于 pthread、port I/O 和 ISA/PCI 设备访问实现 PLC 运行时 HAL
 * 适用于 x86/x64 工控机、PC-based PLC 控制器
 */

#define PLATFORM_LINUX_X86

#include "plc_platform.h"
#include "plc_io.h"
#include "plc_comm.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/io.h>

/* 网络相关 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

/* 串口相关 */
#include <termios.h>
#include <sys/ioctl.h>

/* ========== 临界区 ========== */

static pthread_mutex_t g_critical_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_iopl_enabled = 0;

/* ========== 平台初始化 ========== */

void plc_platform_init(void)
{
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  /* 尝试启用 I/O 端口访问权限 (需要 root) */
  if (iopl(3) == 0) {
    g_iopl_enabled = 1;
    plc_platform_log(PLC_LOG_INFO, "I/O 端口访问已启用 (iopl=3)");
  } else {
    plc_platform_log(PLC_LOG_WARN, "无法启用 I/O 端口访问，使用 sysfs 备选方案");
  }

  plc_platform_log(PLC_LOG_INFO, "x86 Linux 平台初始化完成");
}

/* ========== 时间函数 ========== */

uint32_t plc_platform_tick_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

uint64_t plc_platform_tick_us(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)(ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000);
}

void plc_platform_delay_ms(uint32_t ms)
{
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000L;
  nanosleep(&ts, NULL);
}

void plc_platform_delay_us(uint32_t us)
{
  /* 短延时使用 busy-wait 以提高精度 */
  if (us <= 100) {
    uint64_t target = plc_platform_tick_us() + us;
    while (plc_platform_tick_us() < target) {
      __asm__ volatile("pause");
    }
  } else {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000L;
    nanosleep(&ts, NULL);
  }
}

/* ========== 临界区 ========== */

void plc_platform_critical_enter(void)
{
  pthread_mutex_lock(&g_critical_mutex);
}

void plc_platform_critical_exit(void)
{
  pthread_mutex_unlock(&g_critical_mutex);
}

/* ========== 内存管理 ========== */

void* plc_platform_malloc(size_t size)
{
  return malloc(size);
}

void plc_platform_free(void* ptr)
{
  free(ptr);
}

/* ========== 日志 ========== */

static const char* g_log_level_names[] = {
  "ERROR", "WARN ", "INFO ", "DEBUG", "TRACE"
};

void plc_platform_log(uint8_t level, const char* fmt, ...)
{
  if (level > PLC_LOG_DEBUG) return;

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  struct tm tm_info;
  localtime_r(&ts.tv_sec, &tm_info);

  fprintf(stderr, "[%04d-%02d-%02d %02d:%02d:%02d.%03ld] [%s] ",
    tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
    tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec,
    ts.tv_nsec / 1000000, g_log_level_names[level]);

  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  fprintf(stderr, "\n");
}

/* ========== I/O 硬件抽象层 ========== */

/*
 * 读取 ISA 端口 (8/16/32位)
 * 仅在 iopl 成功时可用
 */
static inline uint8_t inb_port(uint16_t port)
{
  return g_iopl_enabled ? inb(port) : 0;
}

static inline void outb_port(uint16_t port, uint8_t val)
{
  if (g_iopl_enabled) outb(val, port);
}

/*
 * 通过 sysfs 读写 GPIO（备选方案，适用于没有 iopl 权限的场景）
 */
static int sysfs_gpio_read(uint32_t gpio_num)
{
  char path[64];
  char value = '0';
  int fd;

  snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/value", gpio_num);
  fd = open(path, O_RDONLY);
  if (fd < 0) return -1;

  read(fd, &value, 1);
  close(fd);
  return (value == '1') ? 1 : 0;
}

static void sysfs_gpio_write(uint32_t gpio_num, int value)
{
  char path[64];
  int fd;

  snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/value", gpio_num);
  fd = open(path, O_WRONLY);
  if (fd >= 0) {
    write(fd, value ? "1" : "0", 1);
    close(fd);
  }
}

/*
 * 通过 PCI 配置空间读取设备 BAR 地址
 * physical_addr 格式: (bus << 16) | (dev << 11) | (func << 8) | reg
 */
static uint32_t pci_config_read(uint32_t addr)
{
  uint16_t bus  = (addr >> 16) & 0xFF;
  uint8_t  dev  = (addr >> 11) & 0x1F;
  uint8_t  func = (addr >> 8)  & 0x07;
  uint8_t  reg  = addr & 0xFC;

  uint32_t config_addr = 0x80000000 |
    (bus << 16) | (dev << 11) | (func << 8) | reg;

  outb_port(0xCF8, (uint8_t)(config_addr & 0xFF));
  outb_port(0xCFA, (uint8_t)((config_addr >> 8) & 0xFF));
  outb_port(0xCFB, (uint8_t)((config_addr >> 16) & 0xFF));
  outb_port(0xCFC, (uint8_t)((config_addr >> 24) & 0xFF));

  uint32_t value = inb_port(0xCFC);
  value |= (uint32_t)inb_port(0xCFC + 1) << 8;
  value |= (uint32_t)inb_port(0xCFC + 2) << 16;
  value |= (uint32_t)inb_port(0xCFC + 3) << 24;

  return value;
}

int32_t plc_hal_read_input(uint32_t physical_addr, IoType type)
{
  switch (type) {
    case IO_TYPE_DI:
      /* 优先尝试 ISA 端口读取 (bit 操作) */
      if (g_iopl_enabled && physical_addr < 0x10000) {
        return (inb_port((uint16_t)physical_addr) >> 0) & 0x01;
      }
      /* 回退到 sysfs GPIO */
      return sysfs_gpio_read(physical_addr);

    case IO_TYPE_AI: {
      /* ISA 端口 ADC: 端口号指向 12-bit ADC 数据寄存器 */
      if (g_iopl_enabled && physical_addr < 0x10000) {
        uint16_t port = (uint16_t)physical_addr;
        uint32_t val = inb_port(port);
        val |= (uint32_t)inb_port(port + 1) << 8;
        return (int32_t)(val & 0x0FFF); /* 12位 ADC */
      }
      return 0;
    }
    case IO_TYPE_ENCODER:
    case IO_TYPE_COUNTER:
      /* ISA 计数器: 端口指向 8254 定时/计数器 */
      if (g_iopl_enabled && physical_addr < 0x10000) {
        uint16_t port = (uint16_t)physical_addr;
        uint32_t val = inb_port(port);
        val |= (uint32_t)inb_port(port) << 8;
        val |= (uint32_t)inb_port(port) << 16;
        return (int32_t)val;
      }
      return 0;
    default:
      return 0;
  }
}

void plc_hal_write_output(uint32_t physical_addr, IoType type, int32_t value)
{
  switch (type) {
    case IO_TYPE_DO:
      if (g_iopl_enabled && physical_addr < 0x10000) {
        outb_port((uint16_t)physical_addr, value ? 0x01 : 0x00);
      } else {
        sysfs_gpio_write(physical_addr, value ? 1 : 0);
      }
      break;

    case IO_TYPE_AO:
      /* ISA DAC: 端口号指向 DAC 数据寄存器 */
      if (g_iopl_enabled && physical_addr < 0x10000) {
        uint16_t port = (uint16_t)physical_addr;
        outb_port(port,     (uint8_t)(value & 0xFF));
        outb_port(port + 1, (uint8_t)((value >> 8) & 0xFF));
      }
      break;

    case IO_TYPE_PWM:
      /* PWM 通过 sysfs 实现 */
      {
        char path[128];
        char buf[32];
        int fd;

        snprintf(path, sizeof(path),
          "/sys/class/pwm/pwmchip0/pwm%u/duty_cycle", physical_addr);
        fd = open(path, O_WRONLY);
        if (fd >= 0) {
          int len = snprintf(buf, sizeof(buf), "%d", value);
          write(fd, buf, len);
          close(fd);
        }
      }
      break;

    default:
      break;
  }
}

/* ========== TCP 通信 ========== */

int plc_hal_tcp_connect(const char* host, uint16_t port, uint32_t timeout_ms)
{
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd < 0) return -1;

  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, host, &addr.sin_addr);

  int ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
  if (ret < 0 && errno != EINPROGRESS) {
    close(fd);
    return -1;
  }

  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLOUT;
  ret = poll(&pfd, 1, (int)timeout_ms);

  if (ret <= 0) {
    close(fd);
    return -1;
  }

  int err = 0;
  socklen_t len = sizeof(err);
  getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
  if (err) {
    close(fd);
    return -1;
  }

  fcntl(fd, F_SETFL, flags);

  int opt = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

  return fd;
}

void plc_hal_tcp_close(int fd)
{
  if (fd >= 0) close(fd);
}

int plc_hal_tcp_send(int fd, const uint8_t* data, uint32_t len)
{
  uint32_t sent = 0;
  while (sent < len) {
    ssize_t n = send(fd, data + sent, len - sent, MSG_NOSIGNAL);
    if (n < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    sent += (uint32_t)n;
  }
  return (int)sent;
}

int plc_hal_tcp_recv(int fd, uint8_t* data, uint32_t max_len, uint32_t timeout_ms)
{
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;

  int ret = poll(&pfd, 1, (int)timeout_ms);
  if (ret <= 0) return (ret == 0) ? 0 : -1;

  ssize_t n = recv(fd, data, max_len, 0);
  return (n >= 0) ? (int)n : -1;
}

/* ========== 串口通信 ========== */

int plc_hal_serial_open(const char* port_name, uint32_t baud_rate,
                        uint8_t data_bits, uint8_t stop_bits, uint8_t parity)
{
  int fd = open(port_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0) return -1;

  struct termios tty;
  memset(&tty, 0, sizeof(tty));

  speed_t speed;
  switch (baud_rate) {
    case 9600:   speed = B9600;   break;
    case 19200:  speed = B19200;  break;
    case 38400:  speed = B38400;  break;
    case 57600:  speed = B57600;  break;
    case 115200: speed = B115200; break;
    case 230400: speed = B230400; break;
    case 460800: speed = B460800; break;
    case 921600: speed = B921600; break;
    default:     speed = B115200; break;
  }
  cfsetispeed(&tty, speed);
  cfsetospeed(&tty, speed);

  tty.c_cflag &= ~CSIZE;
  switch (data_bits) {
    case 5: tty.c_cflag |= CS5; break;
    case 6: tty.c_cflag |= CS6; break;
    case 7: tty.c_cflag |= CS7; break;
    default: tty.c_cflag |= CS8; break;
  }

  if (stop_bits == 2)
    tty.c_cflag |= CSTOPB;
  else
    tty.c_cflag &= ~CSTOPB;

  switch (parity) {
    case 1: /* 偶校验 */
      tty.c_cflag |= PARENB;
      tty.c_cflag &= ~PARODD;
      break;
    case 2: /* 奇校验 */
      tty.c_cflag |= PARENB | PARODD;
      break;
    default:
      tty.c_cflag &= ~PARENB;
      break;
  }

  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~CRTSCTS;

  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
  tty.c_oflag &= ~OPOST;
  tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

  tty.c_cc[VMIN]  = 0;
  tty.c_cc[VTIME] = 1;

  tcsetattr(fd, TCSANOW, &tty);
  tcflush(fd, TCIOFLUSH);

  return fd;
}

void plc_hal_serial_close(int fd)
{
  if (fd >= 0) {
    tcdrain(fd);
    close(fd);
  }
}

int plc_hal_serial_send(int fd, const uint8_t* data, uint32_t len)
{
  ssize_t n = write(fd, data, len);
  if (n < 0) return -1;
  tcdrain(fd);
  return (int)n;
}

int plc_hal_serial_recv(int fd, uint8_t* data, uint32_t max_len, uint32_t timeout_ms)
{
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;

  int ret = poll(&pfd, 1, (int)timeout_ms);
  if (ret <= 0) return (ret == 0) ? 0 : -1;

  ssize_t n = read(fd, data, max_len);
  return (n >= 0) ? (int)n : -1;
}
