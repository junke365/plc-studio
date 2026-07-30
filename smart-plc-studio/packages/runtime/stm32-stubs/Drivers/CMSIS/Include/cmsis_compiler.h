#ifndef CMSIS_COMPILER_H
#define CMSIS_COMPILER_H
#define __ASM __asm
#define __INLINE inline
#define __STATIC_INLINE static inline
#define __ALIGNED(x) __attribute__((aligned(x)))
#define __WEAK __attribute__((weak))
#define __NO_RETURN __attribute__((noreturn))
#define __PACKED __attribute__((packed))
#define __PACKED_STRUCT struct __attribute__((packed))
#define __UNALIGNED_UINT32(x) (*((uint32_t*)(x)))
#endif
