/* ============================================================================
 * STM32F407VGTx 链接脚本
 * MCU:   STM32F407VG (Cortex-M4F)
 * Flash: 0x08000000 - 0x080FFFFF (1 MB)
 * SRAM1: 0x20000000 - 0x2001BFFF (112 KB)
 * SRAM2: 0x2001C000 - 0x2001FFFF (16 KB)
 * CCM:   0x10000000 - 0x1000FFFF (64 KB, 仅 CPU 访问)
 * ============================================================================
 * PLC 运行时内存布局:
 *   Flash 前半 → .text/.rodata (代码+常量)
 *   Flash 后半 → .varmap      (非易失变量存储)
 *   SRAM1      → .data/.bss/.heap/.stack
 *   SRAM2      → 过程映像区 (PII/PIO)
 *   CCM        → FreeRTOS 内核数据结构
 * ===========================================================================*/

MEMORY
{
  FLASH  (rx) : ORIGIN = 0x08000000, LENGTH = 1M
  SRAM1  (rw) : ORIGIN = 0x20000000, LENGTH = 112K
  SRAM2  (rw) : ORIGIN = 0x2001C000, LENGTH = 16K
  CCM    (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

/* 栈和堆大小 */
__stack_size  = 4K;
__heap_size   = 32K;

SECTIONS
{
  /* ========== 中断向量表 ========== */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH

  /* ========== 代码 ========== */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text.*)
    *(.glue_7)
    *(.glue_7t)
    *(.gnu.linkonce.t.*)

    /* 构造函数/析构函数 (C++ 支持) */
    . = ALIGN(4);
    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
    _etext = .;
  } > FLASH

  /* ========== 只读数据 ========== */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    . = ALIGN(4);
  } > FLASH

  /* ARM 异常处理表 */
  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > FLASH

  /* ========== 预初始化数据 ========== */
  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } > FLASH

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } > FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } > FLASH

  /* ========== SRAM1: 数据段 ========== */
  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
    . = ALIGN(4);
    _edata = .;
  } > SRAM1 AT > FLASH

  /* ========== SRAM1: BSS 段 ========== */
  . = ALIGN(4);
  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } > SRAM1

  /* ========== 堆 ========== */
  .heap :
  {
    . = ALIGN(8);
    _heap_start = .;
    . = . + __heap_size;
    . = ALIGN(8);
    _heap_end = .;
  } > SRAM1

  /* ========== 栈 ========== */
  .stack (ORIGIN(SRAM1) + LENGTH(SRAM1) - __stack_size) :
  {
    . = ALIGN(8);
    _stack_top = .;
    . = . + __stack_size;
    . = ALIGN(8);
    _stack_end = .;
  } > SRAM1

  /* ========== SRAM2: PLC 过程映像区 ========== */
  .plc_io (NOLOAD) :
  {
    . = ALIGN(4);
    _plc_pi_start = .;
    *(.plc_pi)
    . = ALIGN(4);
    _plc_pi_end = .;
  } > SRAM2

  /* ========== CCM: FreeRTOS 内核数据 ========== */
  .ccm (NOLOAD) :
  {
    . = ALIGN(256);
    _ccm_start = .;
    *(.ccm)
    . = ALIGN(256);
    _ccm_end = .;
  } > CCM

  /* ========== 丢弃的段 ========== */
  /DISCARD/ :
  {
    libc.a (*)
    libm.a (*)
    libgcc.a (*)
  }

  /* ========== 栈顶符号 ========== */
  PROVIDE(__initial_sp = _stack_top);
  PROVIDE(_estack = _stack_top);
  PROVIDE(__heap_start__ = _heap_start);
  PROVIDE(__heap_end__ = _heap_end);
}
