/**
 * litex/platform.c - LiteX FPGA 平台适配
 *
 * 基于 LiteX RISC-V SoC (vexriscv) + LiteDRAM + LiteX Timer
 * 实现 PLC 运行时 HAL。
 *
 * 硬件假设：
 *   - CPU: VexRiscv (RV32IM)
 *   - 总线: Wishbone 内存映射
 *   - 定时器: LiteX Timer @ CSR_TIMER_BASE (默认 0xf0001000)
 *   - 帧缓冲: 通过 LiteX Video 或直接写入 DDR
 *   - 中断: PLIC (Platform-Level Interrupt Controller)
 *
 * 构建需求：
 *   - RISC-V GCC 交叉编译器 (riscv32-unknown-elf-gcc)
 *   - LiteX 生成的 .h 头文件（csr.h, mem.h 等）
 *   - 链接脚本 (mem.ld) 来自 LiteX 构建输出
 */

#define PLATFORM_LITEX

#include "plc_platform.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* ========== LiteX CSR 寄存器 (由 LiteX 生成, 位于 csr.h) ========== */

#ifdef __has_include
#if __has_include("csr.h")
#include "csr.h"
#else
/* 默认 LiteX CSR 地址（当没有 csr.h 时使用硬编码值） */
#define CSR_TIMER_BASE      0xf0001000UL
#define CSR_UART_BASE       0xf0002000UL
#define CSR_PLIC_BASE       0xf0c00000UL
#endif
#endif

#ifndef CSR_TIMER_BASE
#define CSR_TIMER_BASE      0xf0001000UL
#endif

/* Timer 寄存器偏移（LiteX Timer 标准布局） */
#define TIMER_LOAD          0x00
#define TIMER_RELOAD        0x04
#define TIMER_EN            0x08
#define TIMER_UPDATE_VALUE  0x0C
#define TIMER_VALUE         0x10
#define TIMER_EV_STATUS     0x14
#define TIMER_EV_PENDING    0x18
#define TIMER_EV_ENABLE     0x1C

/* UART 寄存器偏移 */
#define UART_RXTX           0x00
#define UART_TXFULL         0x04
#define UART_RXEMPTY        0x08
#define UART_EV_STATUS      0x0C
#define UART_EV_PENDING     0x10
#define UART_EV_ENABLE      0x14
#define UART_TXEMPTY        0x18

/* ========== CSR 读写宏 ========== */

#define CSR_RW(addr)        (*((volatile uint32_t*)(addr)))
#define CSR_ADDR(base, off) ((uint32_t)((base) + (off)))

/* ========== 内部状态 ========== */

static volatile uint32_t g_tick_counter = 0;
static uint32_t g_timer_freq_hz = 100000000; /* 默认 100MHz，由链接脚本指定 */

/* ========== 定时器中断处理 ========== */

void timer_handler(void) __attribute__((interrupt));
void timer_handler(void)
{
  /* 清除定时器中断 */
  CSR_RW(CSR_ADDR(CSR_TIMER_BASE, TIMER_EV_PENDING)) = 1;
  g_tick_counter++;
}

/* ========== 平台初始化 ========== */

void plc_platform_init(void)
{
  /* 设置定时器频率（如果 linker 提供了频率定义） */
  #ifdef CONFIG_CLOCK_FREQUENCY
    g_timer_freq_hz = CONFIG_CLOCK_FREQUENCY;
  #endif

  /* 初始化 LiteX Timer: 每毫秒产生一次中断 */
  uint32_t tick_val = g_timer_freq_hz / 1000; /* 1ms */
  CSR_RW(CSR_ADDR(CSR_TIMER_BASE, TIMER_LOAD))  = tick_val;
  CSR_RW(CSR_ADDR(CSR_TIMER_BASE, TIMER_RELOAD)) = tick_val;
  CSR_RW(CSR_ADDR(CSR_TIMER_BASE, TIMER_EN))    = 1;

  /* 使能定时器中断 */
  CSR_RW(CSR_ADDR(CSR_TIMER_BASE, TIMER_EV_ENABLE)) = 1;

  /* 全局中断使能（RISC-V mstatus.MIE） */
  __asm__ volatile("csrsi mstatus, 8");

  plc_platform_log(PLC_LOG_INFO, "LiteX FPGA 平台初始化完成, CPU=%luHz",
                   (unsigned long)g_timer_freq_hz);
}

/* ========== 时间函数 ========== */

uint32_t plc_platform_tick_ms(void)
{
  uint32_t tick;
  __asm__ volatile("" ::: "memory");
  tick = g_tick_counter;
  __asm__ volatile("" ::: "memory");
  return tick;
}

uint64_t plc_platform_tick_us(void)
{
  /* 读取定时器当前值以获得微秒精度 */
  CSR_RW(CSR_ADDR(CSR_TIMER_BASE, TIMER_UPDATE_VALUE)) = 1;
  uint32_t remain = CSR_RW(CSR_ADDR(CSR_TIMER_BASE, TIMER_VALUE));
  uint32_t us_per_tick = 1000000 / (g_timer_freq_hz / 1000);
  uint64_t ms_part = (uint64_t)plc_platform_tick_ms() * 1000;
  uint64_t us_part = (uint64_t)remain * us_per_tick / (g_timer_freq_hz / 1000);
  return ms_part + us_part;
}

void plc_platform_delay_ms(uint32_t ms)
{
  uint32_t start = plc_platform_tick_ms();
  while ((plc_platform_tick_ms() - start) < ms) {
    __asm__ volatile("nop");
  }
}

void plc_platform_delay_us(uint32_t us)
{
  uint64_t start = plc_platform_tick_us();
  while ((plc_platform_tick_us() - start) < us) {
    __asm__ volatile("nop");
  }
}

/* ========== 临界区 ========== */

void plc_platform_critical_enter(void)
{
  /* 禁用机器模式中断 */
  __asm__ volatile("csrci mstatus, 8" ::: "memory");
}

void plc_platform_critical_exit(void)
{
  /* 启用机器模式中断 */
  __asm__ volatile("csrsi mstatus, 8" ::: "memory");
}

/* ========== 内存管理 ========== */

/* 简单的堆管理（LiteX 裸机下无 RTOS） */
static uint8_t g_heap[64 * 1024]; /* 64KB 堆 */
static uint32_t g_heap_ptr = 0;

void* plc_platform_malloc(size_t size)
{
  if (g_heap_ptr + size > sizeof(g_heap)) return NULL;
  void* ptr = &g_heap[g_heap_ptr];
  g_heap_ptr += (size + 3) & ~3; /* 4 字节对齐 */
  return ptr;
}

void plc_platform_free(void* ptr)
{
  /* LiteX 裸机下不释放内存 */
  (void)ptr;
}

/* ========== 日志 ========== */

static const char* g_log_level_names[] = {
  "ERROR", "WARN ", "INFO ", "DEBUG", "TRACE"
};

/* UART 轮询发送字节 */
static void uart_putc(char c)
{
  /* 等待发送 FIFO 不满 */
  while (CSR_RW(CSR_ADDR(CSR_UART_BASE, UART_TXFULL))) {
    __asm__ volatile("nop");
  }
  CSR_RW(CSR_ADDR(CSR_UART_BASE, UART_RXTX)) = (uint8_t)c;
}

static void uart_puts(const char* s)
{
  while (*s) {
    if (*s == '\n') uart_putc('\r');
    uart_putc(*s++);
  }
}

void plc_platform_log(uint8_t level, const char* fmt, ...)
{
  if (level > PLC_LOG_DEBUG) return;

  char buf[256];
  int offset = 0;

  uint32_t tick = plc_platform_tick_ms();
  uint32_t ms = tick % 1000;
  uint32_t sec_total = tick / 1000;
  uint32_t s = sec_total % 60;
  uint32_t m = (sec_total / 60) % 60;
  uint32_t h = (sec_total / 3600) % 24;

  offset += snprintf(buf + offset, sizeof(buf) - offset,
    "[%02lu:%02lu:%02lu.%03lu] [%s] ",
    (unsigned long)h, (unsigned long)m, (unsigned long)s,
    (unsigned long)ms, g_log_level_names[level]);

  va_list args;
  va_start(args, fmt);
  offset += vsnprintf(buf + offset, sizeof(buf) - offset, fmt, args);
  va_end(args);

  buf[offset] = '\0';
  uart_puts(buf);
  uart_putc('\n');
}
