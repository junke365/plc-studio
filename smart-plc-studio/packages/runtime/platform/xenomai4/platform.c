/**
 * xenomai4/platform.c - Xenomai 4 Cobalt 实时平台适配
 *
 * 基于 Xenomai 4 Cobalt 实时框架实现 PLC 运行时 HAL
 * 使用 RT-safe 系统调用，提供确定性调度和微秒级精度
 * 适用于需要硬实时保证的工业控制场景
 */

#define PLATFORM_XENOMAI4

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
#include <signal.h>
#include <pthread.h>

/* Xenomai 4 Cobalt 头文件 */
#include <cobalt/thread.h>
#include <cobalt/mutex.h>
#include <cobalt/cond.h>
#include <cobalt/timer.h>
#include <cobalt/clock.h>

/* 网络相关（使用 RT-safe socket） */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <poll.h>

/* 串口相关 */
#include <termios.h>
#include <sys/ioctl.h>

/* SPI/UART 工业 I/O */
#include <linux/spi/spidev.h>
#include <linux/serial.h>

/* ========== 临界区 ========== */

static pthread_mutex_t g_critical_mutex = PTHREAD_MUTEX_INITIALIZER;

/* RT 定时器基准 */
static uint64_t g_rt_base_ns = 0;

/* ========== 平台初始化 ========== */

void plc_platform_init(void)
{
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  /* 记录 RT 时钟基准 */
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  g_rt_base_ns = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;

  /* 设置 Cobalt 定时器精度为 1 微秒 */
  struct timespec quantum;
  quantum.tv_sec = 0;
  quantum.tv_nsec = 1000; /* 1us 精度 */
  cobalt_timer_set_quantum(&quantum);

  plc_platform_log(PLC_LOG_INFO, "Xenomai 4 Cobalt 实时平台初始化完成");
}

/* ========== 时间函数 ========== */

uint32_t plc_platform_tick_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t now_ns = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
  return (uint32_t)((now_ns - g_rt_base_ns) / 1000000ULL);
}

uint64_t plc_platform_tick_us(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t now_ns = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
  return (now_ns - g_rt_base_ns) / 1000ULL;
}

void plc_platform_delay_ms(uint32_t ms)
{
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000L;

  /* 使用 CLOCK_MONOTONIC 实现 RT-safe 延时 */
  struct timespec wake;
  clock_gettime(CLOCK_MONOTONIC, &wake);
  wake.tv_sec += ts.tv_sec;
  wake.tv_nsec += ts.tv_nsec;
  if (wake.tv_nsec >= 1000000000L) {
    wake.tv_sec++;
    wake.tv_nsec -= 1000000000L;
  }
  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wake, NULL);
}

void plc_platform_delay_us(uint32_t us)
{
  struct timespec ts;
  ts.tv_sec = us / 1000000;
  ts.tv_nsec = (us % 1000000) * 1000L;

  /* 短延时使用 busy-wait 以保证 RT 确定性 */
  if (us <= 50) {
    uint64_t target = plc_platform_tick_us() + us;
    while (plc_platform_tick_us() < target) {
      __asm__ volatile("pause");
    }
  } else {
    struct timespec wake;
    clock_gettime(CLOCK_MONOTONIC, &wake);
    wake.tv_sec += ts.tv_sec;
    wake.tv_nsec += ts.tv_nsec;
    if (wake.tv_nsec >= 1000000000L) {
      wake.tv_sec++;
      wake.tv_nsec -= 1000000000L;
    }
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wake, NULL);
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

/*
 * RT-safe 内存分配：使用 mmap 获取锁页内存
 * Cobalt 本身不提供 malloc，对 RT 任务使用 mmap
 */
static void* g_mmap_base = NULL;
static size_t g_mmap_offset = 0;
#define RT_HEAP_SIZE (4 * 1024 * 1024) /* 4MB RT 堆 */

static void rt_heap_init(void)
{
  if (g_mmap_base) return;

  g_mmap_base = mmap(NULL, RT_HEAP_SIZE,
    PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS | MAP_LOCKED,
    -1, 0);

  if (g_mmap_base == MAP_FAILED) {
    g_mmap_base = NULL;
    plc_platform_log(PLC_LOG_WARN, "RT mmap 失败，回退到标准 malloc");
  } else {
    g_mmap_offset = 0;
    plc_platform_log(PLC_LOG_INFO, "RT 堆初始化: %dMB @ %p",
      RT_HEAP_SIZE / (1024 * 1024), g_mmap_base);
  }
}

void* plc_platform_malloc(size_t size)
{
  /* 8 字节对齐 */
  size = (size + 7) & ~7;

  if (g_mmap_base) {
    /* 从 RT 堆分配（简单 bump allocator） */
    if (g_mmap_offset + size > RT_HEAP_SIZE) {
      /* RT 堆已满，回退到标准分配 */
      return malloc(size);
    }
    void* ptr = (char*)g_mmap_base + g_mmap_offset;
    g_mmap_offset += size;
    return ptr;
  }

  /* 非 RT 路径：回退到标准 malloc */
  return malloc(size);
}

void plc_platform_free(void* ptr)
{
  /*
   * RT-safe 堆使用 bump allocator，不支持 free
   * 标准 malloc 分配的内存正常释放
   */
  if (!ptr) return;

  if (g_mmap_base &&
      ptr >= g_mmap_base &&
      ptr < (char*)g_mmap_base + RT_HEAP_SIZE) {
    /* RT 堆中的指针，不做释放（bump allocator 特性） */
    return;
  }

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
  clock_gettime(CLOCK_MONOTONIC, &ts);
  struct tm tm_info;
  time_t sec = (time_t)(ts.tv_sec);
  localtime_r(&sec, &tm_info);

  fprintf(stderr, "[%04d-%02d-%02d %02d:%02d:%02d.%03ld] [RT %s] ",
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
 * SPI 总线读写（用于 SPI I/O 扩展器如 MCP23S17、PCF8574 等）
 * physical_addr 格式: (bus << 16) | (cs << 8) | reg
 */
static int spi_transfer(uint8_t bus, uint8_t cs,
                        const uint8_t* tx, uint8_t* rx, uint32_t len)
{
  char dev_path[32];
  snprintf(dev_path, sizeof(dev_path), "/dev/spidev%u.%u", bus, cs);

  int fd = open(dev_path, O_RDWR);
  if (fd < 0) return -1;

  struct spi_ioc_transfer xfer;
  memset(&xfer, 0, sizeof(xfer));
  xfer.tx_buf = (unsigned long)tx;
  xfer.rx_buf = (unsigned long)rx;
  xfer.len = len;
  xfer.speed_hz = 1000000; /* 1MHz */
  xfer.bits_per_word = 8;

  int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);
  close(fd);
  return (ret < 0) ? -1 : 0;
}

/*
 * UART 读写（用于 Modbus RTU 等串口协议）
 */
static int uart_read(int fd, uint8_t* data, uint32_t max_len)
{
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;

  int ret = poll(&pfd, 1, 10); /* 10ms 超时 */
  if (ret <= 0) return 0;

  ssize_t n = read(fd, data, max_len);
  return (n >= 0) ? (int)n : -1;
}

static int uart_write(int fd, const uint8_t* data, uint32_t len)
{
  ssize_t n = write(fd, data, len);
  if (n >= 0) {
    /* 等待发送完成 */
    int drained = 0;
    ioctl(fd, TIOCOUTQ, &drained);
    while (drained > 0) {
      usleep(100);
      ioctl(fd, TIOCOUTQ, &drained);
    }
  }
  return (n >= 0) ? (int)n : -1;
}

int32_t plc_hal_read_input(uint32_t physical_addr, IoType type)
{
  switch (type) {
    case IO_TYPE_DI: {
      /* 通过 SPI I/O 扩展器读取数字输入 */
      uint8_t bus = (physical_addr >> 16) & 0xFF;
      uint8_t cs  = (physical_addr >> 8) & 0xFF;
      uint8_t reg = physical_addr & 0xFF;

      uint8_t tx[2] = { 0x41, reg }; /* 读命令 + 寄存器地址 */
      uint8_t rx[2] = {0};

      if (spi_transfer(bus, cs, tx, rx, 2) == 0) {
        return (int32_t)rx[1];
      }
      return 0;
    }
    case IO_TYPE_AI: {
      /* 通过 SPI ADC（如 MCP3208）读取模拟输入 */
      uint8_t bus = (physical_addr >> 16) & 0xFF;
      uint8_t cs  = (physical_addr >> 8) & 0xFF;
      uint8_t ch  = physical_addr & 0xFF;

      /* MCP3208 命令: 单端模式, 开始于 bit2 */
      uint8_t tx[3] = { 0x06 | (ch >> 2), (ch & 0x03) << 6, 0x00 };
      uint8_t rx[3] = {0};

      if (spi_transfer(bus, cs, tx, rx, 3) == 0) {
        int32_t val = ((rx[1] & 0x0F) << 8) | rx[2];
        return val;
      }
      return 0;
    }
    case IO_TYPE_ENCODER:
    case IO_TYPE_COUNTER:
      /* 编码器/计数器暂不实现，返回 0 */
      return 0;
    default:
      return 0;
  }
}

void plc_hal_write_output(uint32_t physical_addr, IoType type, int32_t value)
{
  switch (type) {
    case IO_TYPE_DO: {
      /* 通过 SPI I/O 扩展器写入数字输出 */
      uint8_t bus = (physical_addr >> 16) & 0xFF;
      uint8_t cs  = (physical_addr >> 8) & 0xFF;
      uint8_t reg = physical_addr & 0xFF;

      uint8_t tx[2] = { 0x40 | reg, (uint8_t)(value & 0xFF) };
      uint8_t rx[2] = {0};
      spi_transfer(bus, cs, tx, rx, 2);
      break;
    }
    case IO_TYPE_AO: {
      /* 通过 SPI DAC（如 MCP4921）写入模拟输出 */
      uint8_t bus = (physical_addr >> 16) & 0xFF;
      uint8_t cs  = (physical_addr >> 8) & 0xFF;
      uint16_t dac_val = (uint16_t)(value & 0x0FFF);

      uint8_t tx[2] = {
        (uint8_t)(0x30 | ((dac_val >> 8) & 0x0F)),
        (uint8_t)(dac_val & 0xFF)
      };
      uint8_t rx[2] = {0};
      spi_transfer(bus, cs, tx, rx, 2);
      break;
    }
    default:
      break;
  }
}

/* ========== TCP 通信（RT-safe socket） ========== */

int plc_hal_tcp_connect(const char* host, uint16_t port, uint32_t timeout_ms)
{
  int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
  if (fd < 0) return -1;

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

  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

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
    case 1:
      tty.c_cflag |= PARENB;
      tty.c_cflag &= ~PARODD;
      break;
    case 2:
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
  return uart_write(fd, data, len);
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
