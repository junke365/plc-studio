/**
 * plc_hmi_input.c - 输入子系统
 *
 * 环形缓冲区管理输入事件，支持多驱动注册
 */

#include "plc_hmi_input.h"
#include <string.h>

/* 事件环形缓冲区 */
static PlcHmiInputEvent g_event_buf[PLC_HMI_INPUT_MAX_EVENTS];
static volatile uint8_t g_buf_head = 0;
static volatile uint8_t g_buf_tail = 0;

/* 输入驱动注册表 */
static const PlcHmiInputDriver* g_input_drivers[PLC_HMI_INPUT_TYPE_COUNT];
static bool g_input_initialized = false;

/* ========== 初始化 ========== */

int plc_hmi_input_init(void)
{
  g_buf_head = 0;
  g_buf_tail = 0;
  memset(g_event_buf, 0, sizeof(g_event_buf));
  memset((void*)g_input_drivers, 0, sizeof(g_input_drivers));
  g_input_initialized = true;
  return 0;
}

/* ========== 缓冲区操作 ========== */

static void buf_push(const PlcHmiInputEvent* event)
{
  uint8_t next = (g_buf_head + 1) % PLC_HMI_INPUT_MAX_EVENTS;
  if (next == g_buf_tail) {
    /* 缓冲区满，丢弃最旧事件 */
    g_buf_tail = (g_buf_tail + 1) % PLC_HMI_INPUT_MAX_EVENTS;
  }
  g_event_buf[g_buf_head] = *event;
  g_buf_head = next;
}

/* ========== 轮询 ========== */

int plc_hmi_input_poll(void)
{
  int count = 0;
  for (int i = 0; i < PLC_HMI_INPUT_TYPE_COUNT; i++) {
    if (g_input_drivers[i] && g_input_drivers[i]->poll_event) {
      PlcHmiInputEvent ev;
      bool has = false;
      g_input_drivers[i]->poll_event(&ev, &has);
      if (has) {
        buf_push(&ev);
        count++;
      }
    }
  }
  return count;
}

/* ========== 获取事件 ========== */

bool plc_hmi_input_get_event(PlcHmiInputEvent* event)
{
  if (g_buf_head == g_buf_tail) return false;
  *event = g_event_buf[g_buf_tail];
  g_buf_tail = (g_buf_tail + 1) % PLC_HMI_INPUT_MAX_EVENTS;
  return true;
}

/* ========== 驱动注册 ========== */

int plc_hmi_input_register_driver(PlcHmiInputType type,
                                   const PlcHmiInputDriver* driver)
{
  if (type >= PLC_HMI_INPUT_TYPE_COUNT || !driver) return -1;
  g_input_drivers[type] = driver;
  if (g_input_initialized && driver->init) {
    driver->init();
  }
  return 0;
}

/* ========== 注入事件 ========== */

void plc_hmi_input_inject(const PlcHmiInputEvent* event)
{
  if (event) buf_push(event);
}

/* ========== 清空 ========== */

void plc_hmi_input_flush(void)
{
  g_buf_head = 0;
  g_buf_tail = 0;
}
