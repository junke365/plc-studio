/**
 * test_runtime_state.c - PLC 运行时状态机测试 (STM32 运行态模拟)
 *
 * 模拟 STM32 main.c 中 plc_scan_task (1ms 扫描) 的运行逻辑，
 * 在主机上用 win32 平台验证运行态的完整生命周期:
 *   1. 状态机: INIT -> LOAD -> START -> RUNNING
 *   2. 扫描循环: plc_runtime_scan (模拟 1ms 周期任务)
 *   3. 变量注册/读写
 *   4. I/O 通道注册与绑定
 *   5. 统计信息: cycle_count / cycle_time_us / uptime_ms
 *   6. 暂停 / 恢复 / 停止 / 复位
 *
 * 编译 (MinGW):
 *   gcc -o test_runtime_state test_runtime_state.c \
 *       ../core/src/*.c ../platform/win32/platform.c \
 *       -I../core/include -I. -lwinmm -lws2_32
 */

#include "plc_runtime.h"
#include "plc_io.h"
#include "plc_var.h"
#include "plc_platform.h"
#include <stdio.h>
#include <string.h>

/* ========== 生成代码存根 (模拟 ST 编译产物) ========== */

static PlcRuntime* s_test_rt = NULL;

void generated_init(PlcVarTable* vt, PlcIoConfig* io)
{
  /* 模拟 ST 程序 Main 程序变量 */
  plc_var_register(vt, "Main.Motor", VAR_TYPE_BOOL, VAR_ATTR_GLOBAL,
                   sizeof(plc_bool), "电机运行标志");
  plc_var_register(vt, "Main.Alarm", VAR_TYPE_BOOL, VAR_ATTR_GLOBAL,
                   sizeof(plc_bool), "报警标志");
  plc_var_register(vt, "Main.RunCount", VAR_TYPE_INT, VAR_ATTR_GLOBAL,
                   sizeof(plc_int), "运行计数");
  plc_var_register(vt, "Main.TempValue", VAR_TYPE_INT, VAR_ATTR_GLOBAL,
                   sizeof(plc_int), "温度值");

  /* 模拟 STM32 硬件 I/O 变量 */
  plc_var_register(vt, "sensor_di_0", VAR_TYPE_BOOL, VAR_ATTR_INPUT,
                   sizeof(plc_bool), "数字量输入 0 (PB0)");
  plc_var_register(vt, "sensor_di_1", VAR_TYPE_BOOL, VAR_ATTR_INPUT,
                   sizeof(plc_bool), "数字量输入 1 (PB1)");
  plc_var_register(vt, "adc_ch_0", VAR_TYPE_UINT, VAR_ATTR_INPUT,
                   sizeof(plc_uint), "模拟量输入 0 (PA0)");
  plc_var_register(vt, "relay_0", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT,
                   sizeof(plc_bool), "继电器 0 (PC2)");
  plc_var_register(vt, "pwm_0", VAR_TYPE_UINT, VAR_ATTR_OUTPUT,
                   sizeof(plc_uint), "PWM 输出 0 (PA6)");

  /* 注册 I/O 通道 (与 STM32 main.c 一致) */
  plc_io_register(io, IO_TYPE_DI,  "DI_0",  "sensor_di_0", 0x10);
  plc_io_register(io, IO_TYPE_DI,  "DI_1",  "sensor_di_1", 0x11);
  plc_io_register(io, IO_TYPE_AI,  "AI_0",  "adc_ch_0",    0x00);
  plc_io_register(io, IO_TYPE_DO,  "DO_0",  "relay_0",     0x22); /* PC2 */
  plc_io_register(io, IO_TYPE_PWM, "PWM_0", "pwm_0",       0x30);
}

void generated_main(void)
{
  /* 模拟 PLC 用户程序: 每次扫描递增 RunCount */
  if (s_test_rt != NULL) {
    PlcVarTable* vt = plc_runtime_get_var_table(s_test_rt);
    plc_int cnt = 0;
    plc_var_read(vt, "Main.RunCount", &cnt, sizeof(cnt));
    cnt++;
    plc_var_write(vt, "Main.RunCount", &cnt, sizeof(cnt));
  }
}

uint32_t generated_pou_count(void) { return 1; }
const char* generated_pou_name(uint32_t idx) { (void)idx; return "Main"; }

/* ========== 测试工具 ========== */

static int g_pass = 0;
static int g_fail = 0;

#define TEST(name) do { printf("  TEST: %s ... ", name); } while(0)
#define PASS() do { printf("PASS\n"); g_pass++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); g_fail++; } while(0)
#define ASSERT(cond, msg) do { \
  if (!(cond)) { FAIL(msg); return; } \
} while(0)

/* ========== 状态机测试 ========== */

static void test_initial_state(void)
{
  TEST("初始状态 INIT");
  PlcRuntime rt;
  plc_runtime_init(&rt);
  ASSERT(plc_runtime_get_state(&rt) == PLC_STATE_INIT, "初始化后应为 INIT");
  ASSERT(plc_var_count(plc_runtime_get_var_table(&rt)) == 0, "初始化后变量应为 0");
  PASS();
}

static void test_load_start(void)
{
  TEST("LOAD -> START -> RUNNING 状态转换");
  PlcRuntime rt;
  plc_runtime_init(&rt);
  ASSERT(plc_runtime_load(&rt) == 0, "load 应成功");
  ASSERT(plc_runtime_get_state(&rt) == PLC_STATE_INIT, "load 后仍为 INIT");
  ASSERT(plc_var_count(plc_runtime_get_var_table(&rt)) == 9, "load 后应有 9 个变量");

  ASSERT(plc_runtime_start(&rt) == 0, "start 应成功");
  ASSERT(plc_runtime_get_state(&rt) == PLC_STATE_RUNNING, "start 后应为 RUNNING");
  PASS();
}

static void test_invalid_transitions(void)
{
  TEST("非法状态转换防护");
  PlcRuntime rt;
  plc_runtime_init(&rt);

  /* INIT 状态 start 应成功 */
  ASSERT(plc_runtime_start(&rt) == 0, "INIT 状态 start 应成功");

  /* RUNNING 状态重复 start 应失败 */
  ASSERT(plc_runtime_start(&rt) != 0, "RUNNING 重复 start 应失败");

  /* INIT 状态 stop 应失败 */
  PlcRuntime rt2;
  plc_runtime_init(&rt2);
  ASSERT(plc_runtime_stop(&rt2) != 0, "INIT 状态 stop 应失败");
  PASS();
}

/* ========== 扫描循环测试 (模拟 STM32 1ms 扫描任务) ========== */

static void test_scan_loop(void)
{
  TEST("扫描循环 500 次 (模拟 1ms 扫描任务)");
  PlcRuntime rt;
  plc_runtime_init(&rt);
  s_test_rt = &rt;
  plc_runtime_load(&rt);
  plc_runtime_start(&rt);

  /* 模拟 plc_scan_task: 1ms 周期调用 plc_runtime_scan */
  for (int i = 0; i < 500; i++) {
    plc_runtime_scan(&rt);
    plc_platform_delay_us(1000);
  }

  PlcStats stats;
  plc_runtime_get_stats(&rt, &stats);
  ASSERT(stats.cycle_count == 500, "应执行 500 个周期");
  ASSERT(stats.cycle_time_us > 0, "周期耗时应 > 0");
  ASSERT(stats.max_cycle_time_us >= stats.cycle_time_us, "最大周期应 >= 最近周期");
  ASSERT(stats.uptime_ms > 0, "运行时长应 > 0");

  /* 验证 generated_main 的 RunCount 递增 */
  plc_int cnt = 0;
  plc_var_read(plc_runtime_get_var_table(&rt), "Main.RunCount", &cnt, sizeof(cnt));
  ASSERT(cnt >= 1, "用户程序应递增 RunCount");

  /* 输出运行态状态 */
  printf("    (state=%d cycles=%lu avg=%luus max=%luus uptime=%lums)\n",
         (int)plc_runtime_get_state(&rt),
         (unsigned long)stats.cycle_count,
         (unsigned long)stats.cycle_time_us,
         (unsigned long)stats.max_cycle_time_us,
         (unsigned long)stats.uptime_ms);

  plc_runtime_stop(&rt);
  PASS();
}

/* ========== 变量读写测试 ========== */

static void test_variable_rw(void)
{
  TEST("变量注册/查找/读写");
  PlcRuntime rt;
  plc_runtime_init(&rt);
  PlcVarTable* vt = plc_runtime_get_var_table(&rt);

  plc_var_register(vt, "Main.Motor", VAR_TYPE_BOOL, VAR_ATTR_GLOBAL,
                   sizeof(plc_bool), NULL);
  plc_var_register(vt, "Main.TempValue", VAR_TYPE_INT, VAR_ATTR_GLOBAL,
                   sizeof(plc_int), NULL);
  plc_var_register(vt, "adc_ch_0", VAR_TYPE_UINT, VAR_ATTR_INPUT,
                   sizeof(plc_uint), NULL);

  ASSERT(plc_var_find(vt, "Main.Motor") != NULL, "查找 Main.Motor 应成功");
  ASSERT(plc_var_find(vt, "不存在") == NULL, "查找不存在变量应返回 NULL");
  ASSERT(plc_var_register(vt, "Main.Motor", VAR_TYPE_BOOL, VAR_ATTR_GLOBAL,
                          sizeof(plc_bool), NULL) == -2, "重复注册应失败");

  /* 写入并读回 */
  plc_bool m = 1;
  ASSERT(plc_var_write(vt, "Main.Motor", &m, sizeof(m)) == 0, "写入应成功");
  plc_bool rd = 0;
  ASSERT(plc_var_read(vt, "Main.Motor", &rd, sizeof(rd)) == (int)sizeof(rd), "读取应成功");
  ASSERT(rd == 1, "读回值应等于写入值");

  plc_int tv = 85;
  plc_var_write(vt, "Main.TempValue", &tv, sizeof(tv));
  plc_int tvRd = 0;
  plc_var_read(vt, "Main.TempValue", &tvRd, sizeof(tvRd));
  ASSERT(tvRd == 85, "INT 读回值应等于 85");

  PASS();
}

/* ========== I/O 绑定测试 ========== */

static void test_io_bind(void)
{
  TEST("I/O 通道绑定到变量");
  PlcRuntime rt;
  plc_runtime_init(&rt);
  plc_runtime_load(&rt);

  PlcIoConfig* io = plc_runtime_get_io_config(&rt);
  PlcVarTable* vt = plc_runtime_get_var_table(&rt);

  /* load 时 generated_init 已注册 5 个 I/O 通道 */
  ASSERT(io->channel_count == 5, "应有 5 个 I/O 通道");

  /* start 时绑定变量 */
  plc_runtime_start(&rt);
  ASSERT(io->channels[0].var != NULL, "DI_0 应绑定到变量");
  ASSERT(io->channels[0].var == plc_var_find(vt, "sensor_di_0"),
         "DI_0 应绑定到 sensor_di_0");

  /* 写入输出变量, 验证映射 */
  plc_bool relay = 1;
  plc_var_write(vt, "relay_0", &relay, sizeof(relay));
  plc_io_write_outputs(io);
  ASSERT(plc_io_read_channel(io, 3) == 1, "DO_0 输出应为 1");

  plc_runtime_stop(&rt);
  PASS();
}

/* ========== 暂停/恢复/停止/复位测试 ========== */

static void test_pause_resume(void)
{
  TEST("暂停/恢复状态转换");
  PlcRuntime rt;
  plc_runtime_init(&rt);
  plc_runtime_start(&rt);
  ASSERT(plc_runtime_get_state(&rt) == PLC_STATE_RUNNING, "start 后 RUNNING");

  ASSERT(plc_runtime_pause(&rt) == 0, "pause 应成功");
  ASSERT(plc_runtime_get_state(&rt) == PLC_STATE_PAUSED, "pause 后 PAUSED");

  /* PAUSED 状态再 pause 应失败 */
  ASSERT(plc_runtime_pause(&rt) != 0, "重复 pause 应失败");

  ASSERT(plc_runtime_resume(&rt) == 0, "resume 应成功");
  ASSERT(plc_runtime_get_state(&rt) == PLC_STATE_RUNNING, "resume 后 RUNNING");
  PASS();
}

static void test_stop_reset(void)
{
  TEST("停止/复位生命周期");
  PlcRuntime rt;
  plc_runtime_init(&rt);
  plc_runtime_load(&rt);
  plc_runtime_start(&rt);

  /* 运行若干周期后停止 */
  for (int i = 0; i < 100; i++) plc_runtime_scan(&rt);

  ASSERT(plc_runtime_stop(&rt) == 0, "stop 应成功");
  ASSERT(plc_runtime_get_state(&rt) == PLC_STATE_STOPPED, "stop 后 STOPPED");

  /* STOPPED 后再 start */
  ASSERT(plc_runtime_start(&rt) == 0, "STOPPED 后重新 start 应成功");

  /* 复位 */
  ASSERT(plc_runtime_reset(&rt) == 0, "reset 应成功");
  ASSERT(plc_runtime_get_state(&rt) == PLC_STATE_STOPPED, "reset 后 STOPPED");

  /* 复位后变量数据清零, 但定义保留 */
  PlcVarTable* vt = plc_runtime_get_var_table(&rt);
  ASSERT(plc_var_count(vt) == 9, "复位后变量定义应保留");
  PASS();
}

/* ========== 看门狗测试 ========== */

static void test_watchdog(void)
{
  TEST("看门狗喂狗计时");
  PlcRuntime rt;
  plc_runtime_init(&rt);
  ASSERT(rt.watchdog_enabled == true, "看门狗默认启用");
  ASSERT(rt.watchdog_timeout_ms == 5000, "看门狗默认超时 5000ms");

  plc_runtime_start(&rt);
  plc_runtime_scan(&rt);
  uint32_t feed = rt.watchdog_last_feed;
  plc_platform_delay_ms(5);
  plc_runtime_scan(&rt);
  ASSERT(rt.watchdog_last_feed > feed, "scan 后喂狗时间应更新");
  plc_runtime_stop(&rt);
  PASS();
}

/* ========== 主入口 ========== */

int main(void)
{
  printf("========================================\n");
  printf("  PLC 运行时状态机测试 (STM32 运行态模拟)\n");
  printf("========================================\n\n");

  plc_platform_init();

  printf("[状态机]\n");
  test_initial_state();
  test_load_start();
  test_invalid_transitions();

  printf("\n[扫描循环]\n");
  test_scan_loop();

  printf("\n[变量管理]\n");
  test_variable_rw();

  printf("\n[I/O 绑定]\n");
  test_io_bind();

  printf("\n[生命周期]\n");
  test_pause_resume();
  test_stop_reset();

  printf("\n[看门狗]\n");
  test_watchdog();

  printf("\n========================================\n");
  printf("  结果: %d 通过, %d 失败\n", g_pass, g_fail);
  printf("========================================\n");

  return g_fail > 0 ? 1 : 0;
}
