/**
 * hmi_sdl.c - SDL2 主入口
 *
 * 基于 SDL2 的事件循环：
 * - SDL 事件 → HMI 输入事件注入
 * - 固定帧率更新驱动显示
 * - 跨平台：Windows / Linux / macOS
 */

#include "plc_hmi.h"
#include "plc_hmi_driver.h"
#include "plc_hmi_widget.h"
#include "plc_hmi_input.h"
#include <SDL.h>
#include <stdio.h>

/* 屏幕尺寸（由 CMake 传入） */
#ifndef PLC_HMI_SCREEN_WIDTH
#define PLC_HMI_SCREEN_WIDTH 800
#endif
#ifndef PLC_HMI_SCREEN_HEIGHT
#define PLC_HMI_SCREEN_HEIGHT 480
#endif

/* 由 plc_hmi_generated.c 提供的函数 */
extern void plc_hmi_screens_init(void);
extern void plc_hmi_screens_update(void* var_table, uint32_t var_table_size);

/* SDL 驱动（在 plc_hmi_driver_sdl.c 中定义） */
extern const PlcHmiDriver g_sdl_driver;

/* SDL 键码 → HMI 键码映射 */
static uint16_t map_sdl_key(SDL_Keycode sdl_key)
{
  switch (sdl_key) {
    case SDLK_UP:       return PLC_HMI_KEY_UP;
    case SDLK_DOWN:     return PLC_HMI_KEY_DOWN;
    case SDLK_LEFT:     return PLC_HMI_KEY_LEFT;
    case SDLK_RIGHT:    return PLC_HMI_KEY_RIGHT;
    case SDLK_RETURN:   return PLC_HMI_KEY_ENTER;
    case SDLK_ESCAPE:   return PLC_HMI_KEY_ESCAPE;
    case SDLK_BACKSPACE: return PLC_HMI_KEY_BACKSPACE;
    case SDLK_TAB:      return PLC_HMI_KEY_TAB;
    case SDLK_SPACE:    return PLC_HMI_KEY_SPACE;
    case SDLK_F1:       return PLC_HMI_KEY_F1;
    case SDLK_F12:      return PLC_HMI_KEY_F12;
    default:
      if (sdl_key >= SDLK_a && sdl_key <= SDLK_z)
        return (uint16_t)(sdl_key - SDLK_a + 'A');
      if (sdl_key >= SDLK_0 && sdl_key <= SDLK_9)
        return (uint16_t)sdl_key;
      return 0;
  }
}

int main(int argc, char* argv[])
{
  (void)argc; (void)argv;

  /* 初始化平台（高精度计时器等） */
  plc_platform_init();

  /* 注册 SDL 显示驱动 */
  plc_hmi_driver_register(PLC_HMI_DRV_SDL2, &g_sdl_driver);

  /* 配置 HMI */
  PlcHmiConfig cfg = {0};
  cfg.screen_width  = PLC_HMI_SCREEN_WIDTH;
  cfg.screen_height = PLC_HMI_SCREEN_HEIGHT;
  cfg.bpp           = 32;
  cfg.fps_target    = 60;

  plc_hmi_init(&cfg);
  plc_hmi_driver_init(PLC_HMI_DRV_SDL2,
    PLC_HMI_SCREEN_WIDTH, PLC_HMI_SCREEN_HEIGHT, 32);

  /* 加载用户生成的屏幕 */
  plc_hmi_screens_init();

  plc_hmi_start();

  /* ==================== SDL 事件循环 ==================== */
  SDL_Event e;
  bool quit = false;

  while (!quit) {
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          quit = true;
          break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
          PlcHmiInputEvent ev = {
            .type    = PLC_HMI_INPUT_MOUSE,
            .x       = (int16_t)e.button.x,
            .y       = (int16_t)e.button.y,
            .key     = 0,
            .pressed = (e.type == SDL_MOUSEBUTTONDOWN)
          };
          plc_hmi_input_inject(&ev);
          break;
        }

        case SDL_MOUSEMOTION:
          if (e.motion.state & SDL_BUTTON_LMASK) {
            PlcHmiInputEvent ev = {
              .type    = PLC_HMI_INPUT_MOUSE,
              .x       = (int16_t)e.motion.x,
              .y       = (int16_t)e.motion.y,
              .key     = 0,
              .pressed = true
            };
            plc_hmi_input_inject(&ev);
          }
          break;

        case SDL_KEYDOWN:
        case SDL_KEYUP: {
          uint16_t key = map_sdl_key(e.key.keysym.sym);
          if (key != 0) {
            PlcHmiInputEvent ev = {
              .type    = PLC_HMI_INPUT_KEYBOARD,
              .x       = 0,
              .y       = 0,
              .key     = key,
              .pressed = (e.type == SDL_KEYDOWN)
            };
            plc_hmi_input_inject(&ev);
          }
          break;
        }

        case SDL_WINDOWEVENT:
          if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
              e.window.event == SDL_WINDOWEVENT_EXPOSED) {
            /* 触发热区重绘（由下次 update 完成） */
          }
          break;
      }
    }

    /* 更新 HMI 引擎（input → 绑定 → 渲染 → 刷新） */
    plc_hmi_screens_update(NULL, 0);
    plc_hmi_update();

    /* 帧率节制 */
    SDL_Delay(1);
  }

  /* 清理 */
  plc_hmi_stop();

  const PlcHmiDriver* drv = plc_hmi_driver_get(PLC_HMI_DRV_SDL2);
  if (drv && drv->deinit) drv->deinit();

  return 0;
}
