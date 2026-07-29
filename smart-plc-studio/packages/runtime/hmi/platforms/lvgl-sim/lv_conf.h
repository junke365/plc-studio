#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH 32
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

#define LV_MEM_SIZE (1024 * 1024)

#define LV_DEF_REFR_PERIOD  33
#define LV_DPI_DEF 130
#define LV_USE_OS LV_OS_NONE

/* SDL 窗口驱动（用于 PC 仿真） */
#define LV_USE_SDL              1
#if LV_USE_SDL
  #define LV_SDL_INCLUDE_PATH   <SDL2/SDL.h>
  #define LV_SDL_RENDER_MODE    LV_DISPLAY_RENDER_MODE_DIRECT
  #define LV_SDL_BUF_COUNT      1
  #define LV_SDL_ACCELERATED    1
  #define LV_SDL_FULLSCREEN     0
  #define LV_SDL_DIRECT_EXIT    1
  #define LV_SDL_MOUSEWHEEL_MODE LV_SDL_MOUSEWHEEL_MODE_ENCODER
#endif

/* 显示硬件加速 */
#define LV_USE_DRAW_SDL 0
#define LV_USE_DRAW_VG_LITE 0
#define LV_USE_DRAW_ARM2D 0

/* ThorVG 矢量图形（不需要，禁用可加速编译） */
#define LV_USE_THORVG_INTERNAL 0
#define LV_USE_THORVG_EXTERNAL 0

/* 总线接口（不需要） */
#define LV_USE_I2C 0
#define LV_USE_SPI 0

/* 字体 */
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_MONTSERRAT_24  1
#define LV_FONT_DEFAULT         &lv_font_montserrat_16

/* 控件（按需开启） */
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
#define LV_USE_DROPDOWN  1
#define LV_USE_CHECKBOX  1
#define LV_USE_TEXTAREA  1
#define LV_USE_KEYBOARD  1
#define LV_USE_ROLLER    1

/* 不要示例和演示 */
#define LV_BUILD_EXAMPLES 0
#define LV_BUILD_DEMOS    0

#endif /* LV_CONF_H */
