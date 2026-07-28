/**
 * hmi_sdl2.c - SDL2显示驱动
 *
 * 通过SDL2实现窗口显示和输入处理：
 * - SDL_CreateWindow / SDL_CreateRenderer
 * - 纹理从帧缓冲区更新（ARGB8888）
 * - 鼠标事件 → 触摸输入
 * - 键盘事件 → HMI输入
 */

#include "plc_hmi.h"
#include "plc_hmi_driver.h"
#include "plc_hmi_input.h"

#ifdef PLC_USE_SDL2

#include <SDL2/SDL.h>
#include <string.h>

/* ========== 全局变量 ========== */

static SDL_Window*   g_window = NULL;
static SDL_Renderer* g_renderer = NULL;
static SDL_Texture*  g_texture = NULL;
static uint16_t g_sdl_w = 800;
static uint16_t g_sdl_h = 480;

/* ========== SDL2驱动实现 ========== */

static int sdl2_init(uint16_t w, uint16_t h, uint8_t bpp)
{
  (void)bpp;
  g_sdl_w = w;
  g_sdl_h = h;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "[HMI-SDL2] SDL初始化失败: %s\n", SDL_GetError());
    return -1;
  }

  g_window = SDL_CreateWindow(
    "Smart PLC HMI",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    w, h,
    SDL_WINDOW_SHOWN
  );
  if (!g_window) {
    fprintf(stderr, "[HMI-SDL2] 创建窗口失败: %s\n", SDL_GetError());
    return -2;
  }

  g_renderer = SDL_CreateRenderer(g_window, -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!g_renderer) {
    /* 回退到软件渲染 */
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (!g_renderer) {
    fprintf(stderr, "[HMI-SDL2] 创建渲染器失败: %s\n", SDL_GetError());
    return -3;
  }

  /* 创建纹理（从帧缓冲区更新） */
  g_texture = SDL_CreateTexture(g_renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    w, h);
  if (!g_texture) {
    fprintf(stderr, "[HMI-SDL2] 创建纹理失败: %s\n", SDL_GetError());
    return -4;
  }

  SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);

  printf("[HMI-SDL2] 初始化成功: %dx%d\n", w, h);
  return 0;
}

static void sdl2_deinit(void)
{
  if (g_texture)  { SDL_DestroyTexture(g_texture);  g_texture = NULL; }
  if (g_renderer) { SDL_DestroyRenderer(g_renderer); g_renderer = NULL; }
  if (g_window)   { SDL_DestroyWindow(g_window);     g_window = NULL; }
  SDL_Quit();
}

static void sdl2_flush(const void* fb, uint16_t w, uint16_t h, uint8_t bpp)
{
  (void)bpp;
  if (!g_texture || !fb) return;

  SDL_UpdateTexture(g_texture, NULL, fb, (int)w * 4);
  SDL_RenderClear(g_renderer);
  SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
  SDL_RenderPresent(g_renderer);
}

static uint16_t sdl2_get_width(void)  { return g_sdl_w; }
static uint16_t sdl2_get_height(void) { return g_sdl_h; }

static const PlcHmiDriver g_sdl2_driver = {
  "sdl2",
  sdl2_init,
  sdl2_deinit,
  sdl2_flush,
  sdl2_get_width,
  sdl2_get_height
};

/* ========== 公共接口 ========== */

int plc_hmi_sdl2_register(void)
{
  plc_hmi_driver_register(PLC_HMI_DRV_SDL2, &g_sdl2_driver);
  return 0;
}

/**
 * 处理SDL事件队列
 * @return 0正常, 非0需要退出
 */
int plc_hmi_sdl2_poll_events(void)
{
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        return 1;

      case SDL_MOUSEBUTTONDOWN: {
        PlcHmiInputEvent ev = {
          PLC_HMI_INPUT_MOUSE,
          (int16_t)event.button.x, (int16_t)event.button.y,
          0, true
        };
        plc_hmi_input_inject(&ev);
        break;
      }

      case SDL_MOUSEBUTTONUP: {
        PlcHmiInputEvent ev = {
          PLC_HMI_INPUT_MOUSE,
          (int16_t)event.button.x, (int16_t)event.button.y,
          0, false
        };
        plc_hmi_input_inject(&ev);
        break;
      }

      case SDL_MOUSEMOTION: {
        if (event.motion.state & SDL_BUTTON_LMASK) {
          PlcHmiInputEvent ev = {
            PLC_HMI_INPUT_MOUSE,
            (int16_t)event.motion.x, (int16_t)event.motion.y,
            0, true
          };
          plc_hmi_input_inject(&ev);
        }
        break;
      }

      case SDL_KEYDOWN: {
        uint16_t key = event.key.keysym.sym;
        /* 映射特殊键 */
        switch (event.key.keysym.sym) {
          case SDLK_UP:    key = PLC_HMI_KEY_UP; break;
          case SDLK_DOWN:  key = PLC_HMI_KEY_DOWN; break;
          case SDLK_LEFT:  key = PLC_HMI_KEY_LEFT; break;
          case SDLK_RIGHT: key = PLC_HMI_KEY_RIGHT; break;
          case SDLK_RETURN: key = PLC_HMI_KEY_ENTER; break;
          case SDLK_ESCAPE: key = PLC_HMI_KEY_ESCAPE; break;
          case SDLK_F1:    key = PLC_HMI_KEY_F1; break;
          case SDLK_F12:   key = PLC_HMI_KEY_F12; break;
        }
        PlcHmiInputEvent ev = {PLC_HMI_INPUT_KEYBOARD, 0, 0, key, true};
        plc_hmi_input_inject(&ev);
        break;
      }

      case SDL_KEYUP: {
        uint16_t key = event.key.keysym.sym;
        PlcHmiInputEvent ev = {PLC_HMI_INPUT_KEYBOARD, 0, 0, key, false};
        plc_hmi_input_inject(&ev);
        break;
      }

      default:
        break;
    }
  }
  return 0;
}

#endif /* PLC_USE_SDL2 */
