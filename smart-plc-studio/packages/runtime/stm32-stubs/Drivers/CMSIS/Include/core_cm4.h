#ifndef CORE_CM4_H
#define CORE_CM4_H
#include <stdint.h>

typedef struct {
  volatile uint32_t ISER[8];
  uint32_t RESERVED0[24];
  volatile uint32_t ICER[8];
  uint32_t RSERVED1[24];
  volatile uint32_t ISPR[8];
  uint32_t RESERVED2[24];
  volatile uint32_t ICPR[8];
  uint32_t RESERVED3[24];
  volatile uint32_t IABR[8];
  uint32_t RESERVED4[56];
  volatile uint8_t  IP[240];
  uint32_t RESERVED5[644];
  volatile uint32_t STIR;
} NVIC_Type;

typedef struct {
  volatile uint32_t CPUID;
  volatile uint32_t ICSR;
  volatile uint32_t VTOR;
  volatile uint32_t AIRCR;
  volatile uint32_t SCR;
  volatile uint32_t CCR;
  volatile uint8_t  SHP[12];
  volatile uint32_t SHCSR;
  volatile uint32_t CFSR;
  volatile uint32_t HFSR;
  volatile uint32_t DFSR;
  volatile uint32_t MMFAR;
  volatile uint32_t BFAR;
  volatile uint32_t AFSR;
  uint32_t RESERVED0[19];
  volatile uint32_t CPACR;
} SCB_Type;

typedef struct {
  volatile uint32_t CTRL;
  volatile uint32_t CYCCNT;
  volatile uint32_t CPICNT;
  volatile uint32_t EXCCNT;
  volatile uint32_t SLEEPCNT;
  volatile uint32_t LSUCNT;
  volatile uint32_t FOLDCNT;
  volatile uint32_t PCSR;
  volatile uint32_t COMP0;
  volatile uint32_t MASK0;
  volatile uint32_t FUNCTION0;
  uint32_t RESERVED[5];
  volatile uint32_t COMP1;
  volatile uint32_t MASK1;
  volatile uint32_t FUNCTION1;
} DWT_Type;

typedef struct {
  volatile uint32_t DHCSR;
  volatile uint32_t DCRSR;
  volatile uint32_t DCRDR;
  volatile uint32_t DEMCR;
} CoreDebug_Type;

#define SCS_BASE            (0xE000E000UL)
#define CoreDebug_BASE      (0xE000EDF0UL)
#define NVIC_BASE           (SCS_BASE + 0x0100UL)
#define SCB_BASE            (SCS_BASE + 0x0D00UL)
#define DWT_BASE            (SCS_BASE + 0x1000UL)

#define NVIC                ((NVIC_Type*)NVIC_BASE)
#define SCB                 ((SCB_Type*)SCB_BASE)
#define DWT                 ((DWT_Type*)DWT_BASE)
#define CoreDebug           ((CoreDebug_Type*)CoreDebug_BASE)

#define SCB_CPACR_CP10_Pos  20
#define SCB_CPACR_CP11_Pos  22
#define CoreDebug_DEMCR_TRCENA_Msk (1UL << 24)
#define DWT_CTRL_CYCCNTENA_Msk    (1UL << 0)

#define __get_xPSR()        0
#define __get_CONTROL()     0
#define __set_CONTROL(x)    ((void)(x))
#define __WFI()             ((void)0)
#define __WFE()             ((void)0)
#define __SEV()             ((void)0)
#define __enable_irq()      ((void)0)
#define __disable_irq()     ((void)0)
#define __NOP()             ((void)0)
#define __BKPT(x)           ((void)0)
#endif
