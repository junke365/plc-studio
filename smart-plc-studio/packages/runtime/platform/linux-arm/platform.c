/**
 * linux-arm/platform.c - ARM Linux 平台适配
 *
 * 基于 sysfs GPIO/IIO 和 POSIX 线程实现 PLC 运行时 HAL
 * 适用于树莓派、BeagleBone、RK3568 等 ARM Linux 开发板
 */

#define PLATFORM_LINUX_ARM

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
#include <sys/stat.h>
#include <sys/types.h>

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

/* ========== 临界区互斥锁 ========== */

static pthread_mutex_t g_critical_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ========== 平台初始化 ========== */

void plc_platform_init(void)
{
  /* 禁用 stdout 缓冲，确保日志实时输出 */
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  /* 提升进程优先级 */
  nice(-10);

  plc_platform_log(PLC_LOG_INFO, "ARM Linux 平台初始化完成");
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
  struct timespec ts;
  ts.tv_sec = us / 1000000;
  ts.tv_nsec = (us % 1000000) * 1000L;
  nanosleep(&ts, NULL);
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
 * 通过 sysfs GPIO 读取数字输入
 * physical_addr 格式: (gpio_chip << 16) | gpio_num
 * 例如 GPIO0_23 = (0 << 16) | 23 = 23
 * 对于直接指定编号: physical_addr 直接就是 gpio 编号
 */
static int gpio_export(uint32_t gpio_num)
{
  char path[64];
  char value;
  int fd;

  /* 检查是否已导出 */
  snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/direction", gpio_num);
  if (access(path, F_OK) == 0)
    return 0; /* 已导出 */

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (fd < 0) return -1;

  char buf[16];
  int len = snprintf(buf, sizeof(buf), "%u", gpio_num);
  write(fd, buf, len);
  close(fd);

  /* 等待 sysfs 节点创建 */
  usleep(10000);
  return 0;
}

static int gpio_set_direction(uint32_t gpio_num, const char* direction)
{
  char path[64];
  int fd;

  snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/direction", gpio_num);
  fd = open(path, O_WRONLY);
  if (fd < 0) return -1;

  write(fd, direction, strlen(direction));
  close(fd);
  return 0;
}

static int gpio_read_value(uint32_t gpio_num)
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

static int gpio_write_value(uint32_t gpio_num, int value)
{
  char path[64];
  int fd;

  snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/value", gpio_num);
  fd = open(path, O_WRONLY);
  if (fd < 0) return -1;

  write(fd, value ? "1" : "1", 1);
  close(fd);
  return 0;
}

/*
 * 通过 sysfs IIO 读取模拟输入 (ADC)
 * physical_addr 为 IIO 设备的通道编号
 * 路径示例: /sys/bus/iio/devices/iio:device0/in_voltage0_raw
 */
static int32_t adc_read_raw(uint32_t channel)
{
  char path[128];
  char buf[32];
  int fd;

  snprintf(path, sizeof(path),
    "/sys/bus/iio/devices/iio:device0/in_voltage%u_raw", channel);
  fd = open(path, O_RDONLY);
  if (fd < 0) return -1;

  ssize_t n = read(fd, buf, sizeof(buf) - 1);
  close(fd);

  if (n <= 0) return -1;
  buf[n] = '\0';
  return (int32_t)strtol(buf, NULL, 10);
}

/*
 * 通过 sysfs IIO 写入模拟输出 (DAC)
 * physical_addr 为 DAC 通道编号
 */
static int32_t dac_write_raw(uint32_t channel, int32_t value)
{
  char path[128];
  char buf[32];
  int fd;

  snprintf(path, sizeof(path),
    "/sys/bus/iio/devices/iio:device0/out_voltage%u_raw", channel);
  fd = open(path, O_WRONLY);
  if (fd < 0) return -1;

  int len = snprintf(buf, sizeof(buf), "%d", value);
  write(fd, buf, len);
  close(fd);
  return 0;
}

int32_t plc_hal_read_input(uint32_t physical_addr, IoType type)
{
  switch (type) {
    case IO_TYPE_DI: {
      /* 确保 GPIO 已导出并设为输入 */
      gpio_export(physical_addr);
      gpio_set_direction(physical_addr, "in");
      return gpio_read_value(physical_addr);
    }
    case IO_TYPE_AI:
      return adc_read_raw(physical_addr);
    case IO_TYPE_ENCODER:
    case IO_TYPE_COUNTER:
      /* 编码器/计数器暂通过 GPIO 中断计数实现，这里返回当前 GPIO 值 */
      return gpio_read_value(physical_addr);
    default:
      return 0;
  }
}

void plc_hal_write_output(uint32_t physical_addr, IoType type, int32_t value)
{
  switch (type) {
    case IO_TYPE_DO:
      gpio_export(physical_addr);
      gpio_set_direction(physical_addr, "out");
      gpio_write_value(physical_addr, value ? 1 : 0);
      break;
    case IO_TYPE_AO:
      dac_write_raw(physical_addr, value);
      break;
    case IO_TYPE_PWM:
      /* PWM 通过 sysfs pwm 子系统实现 */
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

  /* 设置非阻塞 */
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

  /* 用 poll 等待连接完成 */
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLOUT;
  ret = poll(&pfd, 1, (int)timeout_ms);

  if (ret <= 0) {
    close(fd);
    return -1;
  }

  /* 恢复阻塞模式 */
  fcntl(fd, F_SETFL, flags);

  /* 设置 TCP_NODELAY */
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

  /* 波特率映射 */
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

  /* 数据位 */
  tty.c_cflag &= ~CSIZE;
  switch (data_bits) {
    case 5: tty.c_cflag |= CS5; break;
    case 6: tty.c_cflag |= CS6; break;
    case 7: tty.c_cflag |= CS7; break;
    default: tty.c_cflag |= CS8; break;
  }

  /* 停止位 */
  if (stop_bits == 2)
    tty.c_cflag |= CSTOPB;
  else
    tty.c_cflag &= ~CSTOPB;

  /* 校验位 */
  switch (parity) {
    case 1: /* 偶校验 */
      tty.c_cflag |= PARENB;
      tty.c_cflag &= ~PARODD;
      break;
    case 2: /* 奇校验 */
      tty.c_cflag |= PARENB | PARODD;
      break;
    default: /* 无校验 */
      tty.c_cflag &= ~PARENB;
      break;
  }

  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~CRTSCTS; /* 无硬件流控 */

  /* 原始模式 */
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
  tty.c_oflag &= ~OPOST;
  tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

  /* 读取控制：至少1字节，超时100ms */
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
