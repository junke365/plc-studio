/**
 * plc_hmi_driver_sdl.c - SDL2 显示驱动
 *
 * 基于 SDL2 实现跨平台帧缓冲区显示：
 * - Windows / Linux / macOS
 * - 硬件加速渲染 (OpenGL/Direct3D)
 * - 支持窗口缩放、全屏切换
 */

#include "plc_hmi_driver.h"
#include <SDL.h>
#include <stdio.h>

static SDL_Window*   g_window  = NULL;
static SDL_Renderer* g_renderer = NULL;
static SDL_Texture*  g_texture = NULL;
static uint16_t      g_win_w   = 800;
static uint16_t      g_win_h   = 600;

static int sdl_init(uint16_t w, uint16_t h, uint8_t bpp)
{
  (void)bpp;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    fprintf(stderr, "[SDL] SDL_Init 失败: %s\n", SDL_GetError());
    return -1;
  }

  g_window = SDL_CreateWindow(
    "Smart PLC HMI",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    w, h,
    SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
  );
  if (!g_window) {
    fprintf(stderr, "[SDL] SDL_CreateWindow 失败: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }

  g_renderer = SDL_CreateRenderer(
    g_window, -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );
  if (!g_renderer) {
    fprintf(stderr, "[SDL] SDL_CreateRenderer 失败: %s\n", SDL_GetError());
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return -1;
  }

  g_texture = SDL_CreateTexture(
    g_renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    w, h
  );
  if (!g_texture) {
    fprintf(stderr, "[SDL] SDL_CreateTexture 失败: %s\n", SDL_GetError());
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return -1;
  }

  g_win_w = w;
  g_win_h = h;
  return 0;
}

static void sdl_deinit(void)
{
  if (g_texture)  SDL_DestroyTexture(g_texture);
  if (g_renderer) SDL_DestroyRenderer(g_renderer);
  if (g_window)   SDL_DestroyWindow(g_window);
  SDL_Quit();

  g_texture  = NULL;
  g_renderer = NULL;
  g_window   = NULL;
  g_win_w = 0;
  g_win_h = 0;
}

static void sdl_flush(const void* fb, uint16_t w, uint16_t h, uint8_t bpp)
{
  (void)bpp;
  if (!g_texture || !fb) return;

  SDL_UpdateTexture(g_texture, NULL, fb, w * 4);
  SDL_RenderClear(g_renderer);
  SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
  SDL_RenderPresent(g_renderer);
}

static uint16_t sdl_get_width(void)  { return g_win_w; }
static uint16_t sdl_get_height(void) { return g_win_h; }

const PlcHmiDriver g_sdl_driver = {
  "sdl2",
  sdl_init,
  sdl_deinit,
  sdl_flush,
  sdl_get_width,
  sdl_get_height
};
