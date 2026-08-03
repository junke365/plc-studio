/**
 * test_debug.c - 调试模块 + 调试协议测试
 *
 * 验证内容:
 *   1. 断点管理: 添加/重复/删除/禁用/容量上限
 *   2. 断点命中: hit_count + 回调事件 DBG_EVENT_BREAKPOINT_HIT
 *   3. 单步执行: DBG_EVENT_STEP_COMPLETE
 *   4. 会话控制: start/stop/pause/continue
 *   5. 变量监控: DBG_EVENT_VARIABLE_CHANGE
 *   6. 调试日志: DBG_EVENT_LOG
 *   7. 协议帧: dbg_build_frame / dbg_parse_frame 边界
 *   8. 完整调试会话: 模拟 STM32 debug_comm_task 命令处理
 *
 * 编译 (MinGW):
 *   gcc -o test_debug test_debug.c \
 *       ../core/src/*.c ../platform/win32/platform.c \
 *       -I../core/include -I. -lwinmm -lws2_32
 */

#include "plc_runtime.h"
#include "plc_debug.h"
#include "plc_debug_protocol.h"
#include "plc_platform.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ========== 生成代码存根 ========== */

void generated_init(PlcVarTable* vt, PlcIoConfig* io)
{
  plc_var_register(vt, "Main.Motor", VAR_TYPE_BOOL, VAR_ATTR_GLOBAL,
                   sizeof(plc_bool), "电机标志");
  plc_var_register(vt, "Main.RunCount", VAR_TYPE_INT, VAR_ATTR_GLOBAL,
                   sizeof(plc_int), "运行计数");
  plc_var_register(vt, "Main.TempValue", VAR_TYPE_INT, VAR_ATTR_GLOBAL,
                   sizeof(plc_int), "温度值");
}

void generated_main(void) {}
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

/* ========== 事件回调记录 ========== */

static DebugEventType g_lastEvent = (DebugEventType)-1;
static uint32_t g_eventCount = 0;
static char g_eventLog[4096];

static void dbg_event_reset(void)
{
  g_lastEvent = (DebugEventType)-1;
  g_eventCount = 0;
  g_eventLog[0] = '\0';
}

static void test_callback(DebugEventType event, const void* data, uint32_t size)
{
  g_lastEvent = event;
  g_eventCount++;
  char line[256];
  snprintf(line, sizeof(line), "event=%d size=%u | ", (int)event, size);
  if (g_eventLog[0] != '\0') strncat(g_eventLog, line, sizeof(g_eventLog) - strlen(g_eventLog) - 1);
  else strncpy(g_eventLog, line, sizeof(g_eventLog) - 1);
  (void)data;
}

/* ========== 断点管理测试 ========== */

static void test_breakpoint_add(void)
{
  TEST("断点添加/重复/容量");
  PlcDebugger dbg;
  plc_debug_init(&dbg);

  /* 添加 */
  int idx = plc_debug_add_breakpoint(&dbg, 0, 10);
  ASSERT(idx == 0, "第一个断点索引应为 0");
  ASSERT(dbg.breakpoint_count == 1, "断点数应为 1");

  /* 重复添加返回同索引 */
  idx = plc_debug_add_breakpoint(&dbg, 0, 10);
  ASSERT(idx == 0, "重复添加应返回已有索引");
  ASSERT(dbg.breakpoint_count == 1, "重复添加不应增加数量");

  /* 不同行 */
  idx = plc_debug_add_breakpoint(&dbg, 0, 20);
  ASSERT(idx == 1, "第二个断点索引应为 1");
  ASSERT(dbg.breakpoint_count == 2, "断点数应为 2");

  /* 填满容量 */
  for (int i = 0; i < PLC_MAX_BREAKPOINTS - 2; i++) {
    ASSERT(plc_debug_add_breakpoint(&dbg, 1, (uint32_t)(100 + i)) >= 0, "应能继续添加");
  }
  ASSERT(plc_debug_add_breakpoint(&dbg, 1, 9999) == -1, "超出容量应失败");

  PASS();
}

static void test_breakpoint_remove_disable(void)
{
  TEST("断点删除/禁用/启用");
  PlcDebugger dbg;
  plc_debug_init(&dbg);

  plc_debug_add_breakpoint(&dbg, 0, 10);
  plc_debug_add_breakpoint(&dbg, 0, 20);
  plc_debug_add_breakpoint(&dbg, 0, 30);
  ASSERT(dbg.breakpoint_count == 3, "应添加 3 个断点");

  /* 禁用中间断点 */
  plc_debug_enable_breakpoint(&dbg, 1, false);
  ASSERT(dbg.breakpoints[1].enabled == false, "断点 1 应被禁用");

  /* 删除中间断点, 尾部前移 */
  plc_debug_remove_breakpoint(&dbg, 0, 20);
  ASSERT(dbg.breakpoint_count == 2, "删除后应为 2 个");
  ASSERT(dbg.breakpoints[1].line == 30, "删除后尾部断点前移");

  PASS();
}

/* ========== 断点命中测试 ========== */

static void test_breakpoint_hit(void)
{
  TEST("断点命中 + 回调事件");
  PlcDebugger dbg;
  plc_debug_init(&dbg);

  dbg_event_reset();
  plc_debug_start(&dbg, test_callback, NULL);
  plc_debug_add_breakpoint(&dbg, 0, 42);

  /* 命中 */
  bool hit = plc_debug_check_breakpoint(&dbg, 0, 42);
  ASSERT(hit == true, "应命中断点");
  ASSERT(dbg.breakpoints[0].hit_count == 1, "命中次数应为 1");
  ASSERT(dbg.total_breaks == 1, "总命中数应为 1");
  ASSERT(g_lastEvent == DBG_EVENT_BREAKPOINT_HIT, "应触发 BREAKPOINT_HIT 事件");
  ASSERT(dbg.session.current_line == 42, "当前行应为 42");

  /* 再次命中 */
  hit = plc_debug_check_breakpoint(&dbg, 0, 42);
  ASSERT(hit == true && dbg.breakpoints[0].hit_count == 2, "再次命中次数应为 2");

  /* 未设断点的行 */
  hit = plc_debug_check_breakpoint(&dbg, 0, 99);
  ASSERT(hit == false, "未设断点行不应命中");

  plc_debug_stop(&dbg);
  PASS();
}

/* ========== 单步/会话控制测试 ========== */

static void test_step_session(void)
{
  TEST("单步 + 会话控制");
  PlcDebugger dbg;
  plc_debug_init(&dbg);

  dbg_event_reset();
  plc_debug_start(&dbg, test_callback, NULL);
  ASSERT(dbg.session.active == true, "会话应激活");

  plc_debug_step(&dbg);
  ASSERT(dbg.session.stepping == true, "单步模式应开启");
  ASSERT(g_lastEvent == DBG_EVENT_STEP_COMPLETE, "应触发 STEP_COMPLETE 事件");

  plc_debug_continue(&dbg);
  ASSERT(dbg.session.stepping == false, "继续执行后单步应关闭");

  plc_debug_pause(&dbg);
  ASSERT(dbg.session.active == false, "暂停后会话应非激活");

  plc_debug_stop(&dbg);
  PASS();
}

/* ========== 变量监控/日志测试 ========== */

static void test_var_monitor(void)
{
  TEST("变量监控事件");
  PlcDebugger dbg;
  plc_debug_init(&dbg);
  dbg_event_reset();
  plc_debug_start(&dbg, test_callback, NULL);

  PlcRuntime rt;
  plc_runtime_init(&rt);
  plc_runtime_load(&rt);
  PlcVariable* var = plc_var_find(plc_runtime_get_var_table(&rt), "Main.Motor");
  ASSERT(var != NULL, "应找到变量 Main.Motor");

  plc_debug_notify_variable(&dbg, var);
  ASSERT(g_lastEvent == DBG_EVENT_VARIABLE_CHANGE, "应触发 VARIABLE_CHANGE 事件");

  plc_debug_stop(&dbg);
  PASS();
}

static void test_debug_log(void)
{
  TEST("调试日志输出");
  PlcDebugger dbg;
  plc_debug_init(&dbg);
  dbg_event_reset();

  /* 未激活会话不输出 */
  plc_debug_log(&dbg, PLC_LOG_INFO, "hello %d", 42);
  ASSERT(g_eventCount == 0, "未激活时不回调");

  plc_debug_start(&dbg, test_callback, NULL);
  plc_debug_log(&dbg, PLC_LOG_INFO, "hello %d", 42);
  ASSERT(g_lastEvent == DBG_EVENT_LOG, "应触发 LOG 事件");
  ASSERT(g_eventCount >= 1, "日志应触发回调");

  plc_debug_stop(&dbg);
  PASS();
}

/* ========== 协议帧测试 ========== */

static void test_protocol_frame(void)
{
  TEST("协议帧构建/解析");
  uint8_t buf[DBG_FRAME_MAX_LEN];

  /* PING 空负载 */
  uint32_t len = dbg_build_frame(buf, sizeof(buf), DBG_CMD_PING, NULL, 0);
  ASSERT(len == 4, "PING 帧应为 4 字节");
  ASSERT(buf[0] == DBG_FRAME_HEADER0 && buf[1] == DBG_FRAME_HEADER1, "帧头应正确");
  ASSERT(buf[2] == 0 && buf[3] == DBG_CMD_PING, "长度和命令应正确");

  /* 解析 */
  uint8_t cmd;
  const uint8_t* payload;
  int32_t plen = dbg_parse_frame(buf, len, &cmd, &payload);
  ASSERT(plen == 0 && cmd == DBG_CMD_PING, "解析 PING 应成功");

  /* 带负载 */
  const uint8_t data[] = { 0xAA, 0xBB, 0xCC };
  len = dbg_build_frame(buf, sizeof(buf), DBG_CMD_WRITE_VAR, data, 3);
  plen = dbg_parse_frame(buf, len, &cmd, &payload);
  ASSERT(plen == 3 && cmd == DBG_CMD_WRITE_VAR, "解析带负载帧应成功");
  ASSERT(payload[0] == 0xAA && payload[2] == 0xCC, "负载内容应正确");

  /* 无效帧 */
  uint8_t bad[4] = { 0x00, 0x00, 0x00, 0x00 };
  plen = dbg_parse_frame(bad, sizeof(bad), &cmd, &payload);
  ASSERT(plen == -1, "无效帧头应解析失败");

  /* 不完整帧 */
  uint8_t shortFrame[3] = { DBG_FRAME_HEADER0, DBG_FRAME_HEADER1, 5 };
  plen = dbg_parse_frame(shortFrame, 3, &cmd, &payload);
  ASSERT(plen == -1, "不完整帧应解析失败");

  /* 缓冲区过小 */
  uint8_t small[3];
  len = dbg_build_frame(small, 3, DBG_CMD_PING, NULL, 0);
  ASSERT(len == 0, "缓冲区过小应返回 0");

  PASS();
}
/* ========== 完整调试会话模拟 ========== */

/* 内存环形缓冲: 模拟 UART RX/TX */
static uint8_t g_rxBuf[1024];
static uint32_t g_rxLen = 0;
static uint8_t g_txBuf[1024];
static uint32_t g_txLen = 0;

static void dbg_comm_reset(void)
{
  g_rxLen = 0;
  g_txLen = 0;
}

/* 模拟 STM32 debug_comm_task 收到的帧 */
static void dbg_feed_frame(uint8_t cmd, const uint8_t* payload, uint32_t pay_len)
{
  uint8_t frame[DBG_FRAME_MAX_LEN];
  uint32_t len = dbg_build_frame(frame, sizeof(frame), cmd, payload, pay_len);
  if (g_rxLen + len <= sizeof(g_rxBuf)) {
    memcpy(g_rxBuf + g_rxLen, frame, len);
    g_rxLen += len;
  }
}

/* 模拟 STM32 debug_comm_task 的主处理逻辑 (与 examples/stm32/main.c 一致) */
static void dbg_comm_process(PlcRuntime* rt)
{
  uint8_t rx_buf[DBG_FRAME_MAX_LEN];
  uint32_t rx_pos = 0;
  uint8_t tx_buf[DBG_FRAME_MAX_LEN];

  /* 拷贝接收数据 */
  uint32_t copyLen = g_rxLen < sizeof(rx_buf) ? g_rxLen : sizeof(rx_buf);
  memcpy(rx_buf, g_rxBuf, copyLen);
  rx_pos = copyLen;
  g_rxLen = 0;
  g_txLen = 0;

  /* 解析并处理帧 */
  while (rx_pos >= 4) {
    uint32_t pay_len = rx_buf[2];
    uint32_t frame_len = 4 + pay_len;
    if (rx_pos < frame_len) break;

    uint8_t cmd = rx_buf[3];
    const uint8_t* payload = rx_buf + 4;
    uint32_t sent_len = 0;

    switch (cmd) {
      case DBG_CMD_PING: {
        sent_len = dbg_build_frame(tx_buf, sizeof(tx_buf), DBG_RSP_PONG, NULL, 0);
        break;
      }
      case DBG_CMD_GET_STATUS: {
        PlcStats stats;
        plc_runtime_get_stats(rt, &stats);
        char status_str[64];
        int n = snprintf(status_str, sizeof(status_str),
                         "{\"status\":%d,\"uptime\":%lu,\"cycles\":%lu}",
                         (int)rt->state, (unsigned long)stats.uptime_ms,
                         (unsigned long)stats.cycle_count);
        sent_len = dbg_build_frame(tx_buf, sizeof(tx_buf),
                                   DBG_RSP_STATUS, (const uint8_t*)status_str, (uint32_t)n);
        break;
      }
      case DBG_CMD_READ_VAR: {
        char var_name[64];
        uint32_t name_len = pay_len < sizeof(var_name) - 1 ? pay_len : sizeof(var_name) - 1;
        memcpy(var_name, payload, name_len);
        var_name[name_len] = '\0';
        PlcVariable* var = plc_var_find(&rt->var_table, var_name);
        if (var != NULL) {
          char val_str[32];
          if (var->type == VAR_TYPE_BOOL)
            snprintf(val_str, sizeof(val_str), "%d", *(plc_bool*)var->data);
          else if (var->type == VAR_TYPE_INT)
            snprintf(val_str, sizeof(val_str), "%d", *(plc_int*)var->data);
          else
            snprintf(val_str, sizeof(val_str), "?");
          uint8_t resp_pay[128];
          uint32_t vn_len = (uint32_t)strlen(var_name);
          uint32_t vv_len = (uint32_t)strlen(val_str);
          resp_pay[0] = (uint8_t)(vn_len & 0xFF);
          resp_pay[1] = (uint8_t)((vn_len >> 8) & 0xFF);
          memcpy(resp_pay + 2, var_name, vn_len);
          memcpy(resp_pay + 2 + vn_len, val_str, vv_len);
          sent_len = dbg_build_frame(tx_buf, sizeof(tx_buf),
                                     DBG_RSP_VAR_VALUE, resp_pay, 2 + vn_len + vv_len);
        } else {
          sent_len = dbg_build_frame(tx_buf, sizeof(tx_buf),
                                     DBG_RSP_ERROR, (const uint8_t*)"变量不存在", 12);
        }
        break;
      }
      case DBG_CMD_WRITE_VAR: {
        if (pay_len >= 2) {
          uint32_t name_len = (uint32_t)payload[0] | ((uint32_t)payload[1] << 8);
          if (2 + name_len <= pay_len) {
            char var_name[64];
            uint32_t cn = name_len < sizeof(var_name) - 1 ? name_len : sizeof(var_name) - 1;
            memcpy(var_name, payload + 2, cn);
            var_name[cn] = '\0';
            char val_str[32];
            cn = (pay_len - 2 - name_len) < sizeof(val_str) - 1
                 ? (pay_len - 2 - name_len) : sizeof(val_str) - 1;
            memcpy(val_str, payload + 2 + name_len, cn);
            val_str[cn] = '\0';
            PlcVariable* var = plc_var_find(&rt->var_table, var_name);
            if (var != NULL) {
              if (var->type == VAR_TYPE_BOOL)
                *(plc_bool*)var->data = (val_str[0] == '1') ? 1 : 0;
              else if (var->type == VAR_TYPE_INT)
                *(plc_int*)var->data = (plc_int)atoi(val_str);
              uint8_t resp_pay[128];
              resp_pay[0] = (uint8_t)(name_len & 0xFF);
              resp_pay[1] = (uint8_t)((name_len >> 8) & 0xFF);
              memcpy(resp_pay + 2, var_name, name_len);
              resp_pay[2 + name_len] = 1;
              sent_len = dbg_build_frame(tx_buf, sizeof(tx_buf),
                                         DBG_RSP_VAR_WRITTEN, resp_pay, 3 + name_len);
            }
          }
        }
        if (sent_len == 0) {
          sent_len = dbg_build_frame(tx_buf, sizeof(tx_buf),
                                     DBG_RSP_ERROR, (const uint8_t*)"写入失败", 12);
        }
        break;
      }
      case DBG_CMD_SET_BP: {
        /* payload: id_len(2) + id + path_len(2) + path + line(2) */
        if (pay_len >= 4) {
          uint32_t id_len = (uint32_t)payload[0] | ((uint32_t)payload[1] << 8);
          uint32_t off = 4 + id_len;
          uint32_t line = 0;
          if (off + 2 <= pay_len) {
            line = (uint32_t)payload[off - 2 + 0]; /* 简化: 取 payload 末尾 */
          }
          plc_debug_add_breakpoint(&rt->debugger, 0, line ? line : 42);
        }
        sent_len = dbg_build_frame(tx_buf, sizeof(tx_buf), DBG_RSP_PONG, NULL, 0);
        break;
      }
      case DBG_CMD_STEP:
        plc_debug_step(&rt->debugger);
        sent_len = dbg_build_frame(tx_buf, sizeof(tx_buf), DBG_RSP_STEPPED, NULL, 0);
        break;
      case DBG_CMD_RUN:
        plc_runtime_start(rt);
        sent_len = dbg_build_frame(tx_buf, sizeof(tx_buf), DBG_RSP_PONG, NULL, 0);
        break;
      case DBG_CMD_PAUSE:
        plc_runtime_stop(rt);
        sent_len = dbg_build_frame(tx_buf, sizeof(tx_buf), DBG_RSP_PONG, NULL, 0);
        break;
      default: {
        sent_len = dbg_build_frame(tx_buf, sizeof(tx_buf),
                                   DBG_RSP_ERROR, (const uint8_t*)"未知命令", 12);
        break;
      }
    }

    if (sent_len > 0 && g_txLen + sent_len <= sizeof(g_txBuf)) {
      memcpy(g_txBuf + g_txLen, tx_buf, sent_len);
      g_txLen += sent_len;
    }

    rx_pos -= frame_len;
    memmove(rx_buf, rx_buf + frame_len, rx_pos);
  }
}

/* 解析 TX 缓冲区, 返回命令码和负载长度 */
static int32_t dbg_tx_get_first(uint8_t* out_cmd, const uint8_t** out_payload)
{
  if (g_txLen < 4) return -1;
  const uint8_t* buf = g_txBuf;
  if (buf[0] != DBG_FRAME_HEADER0 || buf[1] != DBG_FRAME_HEADER1) return -1;
  uint32_t pay_len = buf[2];
  if (g_txLen < 4 + pay_len) return -1;
  *out_cmd = buf[3];
  *out_payload = buf + 4;
  return (int32_t)pay_len;
}

static void test_full_debug_session(void)
{
  TEST("完整调试会话 (模拟 UART 调试通信)");
  PlcRuntime rt;
  plc_runtime_init(&rt);
  plc_runtime_load(&rt);

  dbg_comm_reset();

  /* 1. PING -> PONG */
  dbg_feed_frame(DBG_CMD_PING, NULL, 0);
  dbg_comm_process(&rt);
  uint8_t cmd;
  const uint8_t* payload;
  int32_t plen = dbg_tx_get_first(&cmd, &payload);
  ASSERT(plen == 0 && cmd == DBG_RSP_PONG, "PING 应回复 PONG");

  /* 2. WRITE_VAR Main.RunCount = 123 */
  dbg_comm_reset();
  uint8_t wv[64];
  uint32_t nlen = (uint32_t)strlen("Main.RunCount");
  wv[0] = nlen & 0xFF;
  wv[1] = (nlen >> 8) & 0xFF;
  memcpy(wv + 2, "Main.RunCount", nlen);
  memcpy(wv + 2 + nlen, "123", 3);
  dbg_feed_frame(DBG_CMD_WRITE_VAR, wv, 2 + nlen + 3);
  dbg_comm_process(&rt);
  plen = dbg_tx_get_first(&cmd, &payload);
  ASSERT(plen == 3 + nlen && cmd == DBG_RSP_VAR_WRITTEN, "WRITE_VAR 应回复 VAR_WRITTEN");
  ASSERT(payload[2 + nlen] == 1, "写入成功标志应为 1");

  /* 3. READ_VAR 验证写回 */
  dbg_comm_reset();
  dbg_feed_frame(DBG_CMD_READ_VAR, (const uint8_t*)"Main.RunCount", 13);
  dbg_comm_process(&rt);
  plen = dbg_tx_get_first(&cmd, &payload);
  ASSERT(cmd == DBG_RSP_VAR_VALUE, "READ_VAR 应回复 VAR_VALUE");
  ASSERT(plen >= 2, "响应负载应有名称长度");
  uint32_t rn = (uint32_t)payload[0] | ((uint32_t)payload[1] << 8);
  ASSERT(rn == 13, "变量名长度应为 13");
  char valStr[16];
  memcpy(valStr, payload + 2 + rn, (uint32_t)plen - 2 - rn);
  valStr[plen - 2 - rn] = '\0';
  ASSERT(strcmp(valStr, "123") == 0, "读回值应为 123");

  /* 4. GET_STATUS -> STATUS */
  dbg_comm_reset();
  dbg_feed_frame(DBG_CMD_GET_STATUS, NULL, 0);
  dbg_comm_process(&rt);
  plen = dbg_tx_get_first(&cmd, &payload);
  ASSERT(cmd == DBG_RSP_STATUS, "GET_STATUS 应回复 STATUS");
  ASSERT(plen > 0 && payload != NULL, "状态负载应有内容");

  /* 5. READ_VAR 不存在变量 -> ERROR */
  dbg_comm_reset();
  dbg_feed_frame(DBG_CMD_READ_VAR, (const uint8_t*)"NoSuchVar", 9);
  dbg_comm_process(&rt);
  plen = dbg_tx_get_first(&cmd, &payload);
  ASSERT(cmd == DBG_RSP_ERROR, "读取不存在变量应回复 ERROR");

  /* 6. STEP -> STEPPED */
  dbg_comm_reset();
  dbg_feed_frame(DBG_CMD_STEP, NULL, 0);
  dbg_comm_process(&rt);
  plen = dbg_tx_get_first(&cmd, &payload);
  ASSERT(cmd == DBG_RSP_STEPPED, "STEP 应回复 STEPPED");

  /* 7. PAUSE / RUN */
  dbg_comm_reset();
  dbg_feed_frame(DBG_CMD_RUN, NULL, 0);
  dbg_comm_process(&rt);
  plen = dbg_tx_get_first(&cmd, &payload);
  ASSERT(cmd == DBG_RSP_PONG, "RUN 应回复 PONG");
  ASSERT(plc_runtime_get_state(&rt) == PLC_STATE_RUNNING, "RUN 后应为 RUNNING");

  dbg_comm_reset();
  dbg_feed_frame(DBG_CMD_PAUSE, NULL, 0);
  dbg_comm_process(&rt);
  plen = dbg_tx_get_first(&cmd, &payload);
  ASSERT(cmd == DBG_RSP_PONG, "PAUSE 应回复 PONG");
  ASSERT(plc_runtime_get_state(&rt) == PLC_STATE_STOPPED, "PAUSE 后应为 STOPPED");

  PASS();
}

/* ========== 主入口 ========== */

int main(void)
{
  printf("========================================\n");
  printf("  调试模块 + 调试协议测试\n");
  printf("========================================\n\n");

  plc_platform_init();

  printf("[断点管理]\n");
  test_breakpoint_add();
  test_breakpoint_remove_disable();

  printf("\n[断点命中]\n");
  test_breakpoint_hit();

  printf("\n[单步/会话]\n");
  test_step_session();

  printf("\n[变量监控/日志]\n");
  test_var_monitor();
  test_debug_log();

  printf("\n[协议帧]\n");
  test_protocol_frame();

  printf("\n[完整调试会话]\n");
  test_full_debug_session();

  printf("\n========================================\n");
  printf("  结果: %d 通过, %d 失败\n", g_pass, g_fail);
  printf("========================================\n");

  return g_fail > 0 ? 1 : 0;
}
