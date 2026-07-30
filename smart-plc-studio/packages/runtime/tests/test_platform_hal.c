/**
 * test_platform_hal.c — 平台 HAL 层单元测试
 *
 * 测试内容:
 *   1. GPIO 写入/读取/翻转
 *   2. 步进电机脉冲生成
 *   3. 数字量输入输出
 *   4. 平台时间和延时
 *
 * 编译 (Win32):
 *   gcc -o test_hal test_platform_hal.c ../platform/win32/platform.c -I../core/include -I. -lwinmm -lws2_32
 */

#include "plc_platform.h"
#include "plc_io.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

static int testsPassed = 0;
static int testsFailed = 0;

#define TEST(name) do { printf("  TEST: %s ... ", name); } while(0)
#define PASS() do { printf("PASS\n"); testsPassed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); testsFailed++; } while(0)
#define ASSERT(cond, msg) do { \
  if (!(cond)) { FAIL(msg); return; } \
} while(0)

/* ==================== GPIO HAL 测试 ==================== */

static void test_gpio_write_read(void)
{
  TEST("GPIO 写入和读取");

  /* 写高电平 */
  plc_hal_gpio_write(0x10, 1);  /* PB0 */
  int32_t val = plc_hal_gpio_read(0x10);
  ASSERT(val == 1, "应读到高电平");

  /* 写低电平 */
  plc_hal_gpio_write(0x10, 0);
  val = plc_hal_gpio_read(0x10);
  ASSERT(val == 0, "应读到低电平");

  PASS();
}

static void test_gpio_toggle(void)
{
  TEST("GPIO 翻转");

  plc_hal_gpio_write(0x11, 0);  /* PB1 */
  plc_hal_gpio_toggle(0x11);
  ASSERT(plc_hal_gpio_read(0x11) == 1, "翻转后应为高电平");

  plc_hal_gpio_toggle(0x11);
  ASSERT(plc_hal_gpio_read(0x11) == 0, "再次翻转后应为低电平");

  PASS();
}

static void test_gpio_multiple_pins(void)
{
  TEST("GPIO 多引脚独立");

  /* 多个引脚独立操作 */
  plc_hal_gpio_write(0x10, 1);  /* PB0 */
  plc_hal_gpio_write(0x11, 0);  /* PB1 */
  plc_hal_gpio_write(0x12, 1);  /* PB2 */

  ASSERT(plc_hal_gpio_read(0x10) == 1, "PB0 应=1");
  ASSERT(plc_hal_gpio_read(0x11) == 0, "PB1 应=0");
  ASSERT(plc_hal_gpio_read(0x12) == 1, "PB2 应=1");

  PASS();
}

static void test_gpio_invalid_addr(void)
{
  TEST("GPIO 无效地址");

  /* 越界地址应安全返回 */
  int32_t val = plc_hal_gpio_read(0xFFFF);
  ASSERT(val == 0, "越界读应返回 0");

  plc_hal_gpio_write(0xFFFF, 1); /* 不应崩溃 */

  PASS();
}

/* ==================== 步进脉冲测试 ==================== */

static void test_step_pulse(void)
{
  TEST("步进脉冲生成");

  plc_hal_gpio_write(0x14, 0);  /* STEP=PB4 */
  plc_hal_gpio_write(0x15, 0);  /* DIR=PB5 */

  /* 正向脉冲 */
  plc_hal_step_pulse(0x14, 0x15, 1);
  ASSERT(plc_hal_gpio_read(0x15) == 1, "正向: DIR 应=1");
  /* 脉冲结束后 STEP 应为 0 */
  ASSERT(plc_hal_gpio_read(0x14) == 0, "脉冲后 STEP 应=0");

  /* 负向脉冲 */
  plc_hal_step_pulse(0x14, 0x15, 0);
  ASSERT(plc_hal_gpio_read(0x15) == 0, "负向: DIR 应=0");

  PASS();
}

/* ==================== 数字量 I/O 测试 ==================== */

static void test_digital_io(void)
{
  TEST("数字量输入输出");

  /* 写 DO 并验证 DI 读到相同值（通过平台 GPIO 后端） */
  plc_hal_write_output(0x20, IO_TYPE_DO, 1);  /* PC0 */
  ASSERT(plc_hal_read_input(0x20, IO_TYPE_DI) == 1, "DO=1 时 DI 应读到 1");

  plc_hal_write_output(0x20, IO_TYPE_DO, 0);
  ASSERT(plc_hal_read_input(0x20, IO_TYPE_DI) == 0, "DO=0 时 DI 应读到 0");

  PASS();
}

/* ==================== 平台时间测试 ==================== */

static void test_platform_tick(void)
{
  TEST("平台时间戳");

  uint32_t t1 = plc_platform_tick_ms();
  ASSERT(t1 > 0, "时间戳应 > 0");

  /* 等待一小段时间 */
  plc_platform_delay_ms(10);
  uint32_t t2 = plc_platform_tick_ms();
  ASSERT(t2 >= t1 + 10, "延时 10ms 后时间应增加");

  PASS();
}

static void test_platform_delay_us(void)
{
  TEST("微秒延时");

  uint64_t t1 = plc_platform_tick_us();
  plc_platform_delay_us(100);
  uint64_t t2 = plc_platform_tick_us();
  ASSERT(t2 - t1 >= 95, "100μs 延时误差 < 5%");  /* 允差 5μs */

  PASS();
}

/* ==================== 编码器接口测试 ==================== */

static void test_encoder_read(void)
{
  TEST("编码器输入接口");

  /* 编码器当前返回 0（无硬件 TIM，模拟返回 0） */
  int32_t val = plc_hal_read_input(0x00, IO_TYPE_ENCODER);
  /* 不要求特定值，只验证不崩溃 */
  (void)val;

  PASS();
}

/* ==================== 主入口 ==================== */

int main(void)
{
  printf("========================================\n");
  printf("  平台 HAL 层单元测试\n");
  printf("========================================\n\n");

  /* GPIO */
  printf("[GPIO HAL]\n");
  test_gpio_write_read();
  test_gpio_toggle();
  test_gpio_multiple_pins();
  test_gpio_invalid_addr();

  /* 步进脉冲 */
  printf("\n[步进脉冲]\n");
  test_step_pulse();

  /* 数字 I/O */
  printf("\n[数字 I/O]\n");
  test_digital_io();

  /* 平台时间 */
  printf("\n[平台时间]\n");
  test_platform_tick();
  test_platform_delay_us();

  /* 编码器 */
  printf("\n[编码器]\n");
  test_encoder_read();

  /* 汇总 */
  printf("\n========================================\n");
  printf("  结果: %d 通过, %d 失败\n", testsPassed, testsFailed);
  printf("========================================\n");

  return testsFailed > 0 ? 1 : 0;
}
