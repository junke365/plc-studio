#include "lvgl/lvgl.h"
#include "lvgl/drivers/sdl/lv_sdl_window.h"
#include "lvgl/drivers/sdl/lv_sdl_mouse.h"
#include "lvgl/drivers/sdl/lv_sdl_keyboard.h"

#include "plc_hmi.h"
#include "plc_hmi_driver.h"
#include "plc_hmi_widget.h"

#include <SDL.h>
#include <stdio.h>

#ifndef PLC_HMI_SCREEN_WIDTH
#define PLC_HMI_SCREEN_WIDTH 800
#endif
#ifndef PLC_HMI_SCREEN_HEIGHT
#define PLC_HMI_SCREEN_HEIGHT 480
#endif

extern void plc_hmi_demo_init(void);
extern void plc_hmi_demo_update(void* var_table, uint32_t var_table_size);

extern int  plc_hmi_lvgl_sim_init(uint16_t display_w, uint16_t display_h);
extern void plc_hmi_lvgl_create_widgets(lv_obj_t* parent);
extern void plc_hmi_lvgl_tick(void);
extern void plc_hmi_lvgl_sync_from_widgets(void);
extern void plc_hmi_lvgl_sync_to_widgets(void);

int main(int argc, char* argv[])
{
  (void)argc; (void)argv;

  printf("PLC HMI - LVGL v9 Simulator\n");
  printf("============================\n");

  lv_init();

  lv_display_t* disp = lv_sdl_window_create(
    PLC_HMI_SCREEN_WIDTH, PLC_HMI_SCREEN_HEIGHT);
  if (!disp) {
    fprintf(stderr, "SDL 窗口创建失败\n");
    return 1;
  }
  lv_display_set_default(disp);

  lv_indev_t* mouse = lv_sdl_mouse_create();
  lv_indev_set_display(mouse, disp);

  lv_indev_t* kb = lv_sdl_keyboard_create();
  lv_indev_set_display(kb, disp);
  lv_indev_set_group(kb, lv_group_get_default());

  lv_obj_t* screen = lv_screen_active();

  plc_hmi_lvgl_sim_init(PLC_HMI_SCREEN_WIDTH, PLC_HMI_SCREEN_HEIGHT);

  PlcHmiConfig cfg = {0};
  cfg.screen_width  = PLC_HMI_SCREEN_WIDTH;
  cfg.screen_height = PLC_HMI_SCREEN_HEIGHT;
  cfg.bpp           = 32;
  cfg.fps_target    = 60;

  plc_hmi_init(&cfg);
  plc_hmi_driver_init(PLC_HMI_DRV_LVGL,
    PLC_HMI_SCREEN_WIDTH, PLC_HMI_SCREEN_HEIGHT, 32);

  plc_hmi_demo_init();
  plc_hmi_start();

  plc_hmi_lvgl_create_widgets(screen);

  printf("运行中（关闭窗口退出）\n");

  volatile bool running = true;

  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        running = false;
      }
    }

    lv_timer_handler();

    plc_hmi_demo_update(NULL, 0);
    plc_hmi_update();
    plc_hmi_lvgl_sync_from_widgets();
    plc_hmi_lvgl_sync_to_widgets();

    SDL_Delay(16);
  }

  plc_hmi_stop();
  printf("已退出\n");

  return 0;
}
