#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH 32
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

#define LV_MEM_SIZE (64 * 1024)

#define LV_DEF_REFR_PERIOD  33
#define LV_DPI_DEF 130
#define LV_USE_OS LV_OS_NONE

/* 嵌入式：不使用 SDL / 帧缓冲驱动 */
#define LV_USE_SDL              0
#define LV_USE_LINUX_FBDEV      0
#define LV_USE_DRAW_SDL         0
#define LV_USE_DRAW_VG_LITE     0
#define LV_USE_DRAW_ARM2D       0

#define LV_USE_THORVG_INTERNAL  0
#define LV_USE_THORVG_EXTERNAL  0

/* 字体（精简） */
#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_DEFAULT         &lv_font_montserrat_14

/* 控件（按需） */
#define LV_USE_BTN       1
#define LV_USE_LABEL     1
#define LV_USE_SLIDER    1
#define LV_USE_BAR       1
#define LV_USE_SWITCH    1
#define LV_USE_CHART     1
#define LV_USE_SCALE     1
#define LV_USE_IMAGE     1
#define LV_USE_ARC       1
#define LV_USE_LED       1

#define LV_BUILD_EXAMPLES 0
#define LV_BUILD_DEMOS    0

#endif /* LV_CONF_H */
