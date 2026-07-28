/**
 * examples/litex/main.c - LiteX FPGA HMI 示例主程序
 *
 * 在 LiteX 系统上运行 HMI 演示。
 * 编译需要:
 *   - RISC-V GCC: riscv32-unknown-elf-gcc
 *   - LiteX CSR 头文件: csr.h, mem.h (由 LiteX 构建生成)
 *   - LiteX 链接脚本: mem.ld
 *
 * 典型构建命令:
 *   riscv32-unknown-elf-gcc \\
 *     -march=rv32im -mabi=ilp32 \\
 *     -nostdlib -nostartfiles -ffreestanding \\
 *     -T mem.ld \\
 *     -I. -I/path/to/litex/soc \\
 *     -o hmi-litex.elf \\
 *     main.c platform.c hmi_platform.c \\
 *     -L. -lplc-hmi-litex -lplc-hmi
 */

#include "plc_hmi.h"
#include "plc_hmi_driver.h"
#include "plc_hmi_widget.h"

/* 外部函数：加载 HMI 画面（由 plc_hmi_generated.c 或用户提供） */
extern void plc_hmi_screens_init(void);
extern void plc_hmi_screens_update(void* var_table, uint32_t var_table_size);

/* PLC 变量表（模拟） */
static int32_t g_var_table[64];

int main(void)
{
  /* 1. 初始化硬件平台 */
  plc_platform_init();

  /* 2. 初始化 HMI 引擎 */
  PlcHmiConfig cfg = {0};
  cfg.screen_width  = 800;
  cfg.screen_height = 480;
  cfg.bpp           = 32;
  cfg.fps_target    = 60;
  cfg.var_table     = g_var_table;
  cfg.var_table_size = sizeof(g_var_table);

  int ret = plc_hmi_init(&cfg);
  if (ret < 0) {
    plc_platform_log(PLC_LOG_ERROR, "HMI 初始化失败: %d", ret);
    return -1;
  }

  /* 3. 初始化 LiteX 显示驱动 */
  ret = plc_hmi_litex_init(800, 480);
  if (ret < 0) {
    plc_platform_log(PLC_LOG_WARN, "LiteX 显示驱动初始化失败, 回退到 RAW 模式");
  }

  /* 4. 加载用户界面 */
  plc_hmi_screens_init();

  /* 5. 主循环 */
  plc_platform_log(PLC_LOG_INFO, "LiteX HMI 运行中");

  while (1) {
    /* 更新 PLC 变量（用户逻辑） */
    plc_hmi_screens_update(g_var_table, sizeof(g_var_table));

    /* 更新 HMI 渲染 */
    plc_hmi_update();
  }

  return 0;
}
