/**
 * hmi_win32.c - Win32 GDI显示驱动
 *
 * 基于Win32 API实现帧缓冲区显示：
 * - 注册窗口类、创建窗口
 * - WM_PAINT中通过DIB Section刷新画面
 * - 鼠标/键盘事件转换为HMI输入
 * - Timer驱动更新循环
 */

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>

#include "plc_hmi.h"
#include "plc_hmi_driver.h"
#include "plc_hmi_widget.h"
#include "plc_hmi_input.h"
#include "resource.h"

/* ========== 全局变量 ========== */

static HWND    g_hwnd = NULL;
static HDC     g_hdc = NULL;
static HBITMAP g_dib = NULL;
static void*   g_dib_bits = NULL;
static HBRUSH  g_bg_brush = NULL;

static uint16_t g_win_w = 800;
static uint16_t g_win_h = 480;
static uint8_t  g_win_bpp = 32;

static const char* g_class_name = "PlcHmiWin32";
static bool g_driver_ready = false;

/* ========== DIB Section创建 ========== */

static void create_dib(uint16_t w, uint16_t h)
{
  if (g_dib) {
    DeleteObject(g_dib);
    g_dib = NULL;
  }

  BITMAPINFO bmi;
  memset(&bmi, 0, sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = w;
  bmi.bmiHeader.biHeight = -h; /* 自上而下 */
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  g_dib = CreateDIBSection(g_hdc, &bmi, DIB_RGB_COLORS,
                             &g_dib_bits, NULL, 0);
}

/* ========== 窗口过程 ========== */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  switch (msg) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);

      if (g_dib && g_dib_bits) {
        HDC mem_dc = CreateCompatibleDC(hdc);
        HGDIOBJ old_bmp = SelectObject(mem_dc, g_dib);

        /* 从帧缓冲区拷贝到DIB */
        const PlcHmiScreen* scr = plc_hmi_get_screen();
        if (scr && scr->framebuffer) {
          memcpy(g_dib_bits, scr->framebuffer,
                 (uint32_t)g_win_w * g_win_h * 4);
        }

        BitBlt(hdc, 0, 0, g_win_w, g_win_h, mem_dc, 0, 0, SRCCOPY);
        SelectObject(mem_dc, old_bmp);
        DeleteDC(mem_dc);
      }
      EndPaint(hwnd, &ps);
      return 0;
    }

    case WM_LBUTTONDOWN: {
      int16_t x = (int16_t)LOWORD(lp);
      int16_t y = (int16_t)HIWORD(lp);
      PlcHmiInputEvent ev = {PLC_HMI_INPUT_MOUSE, x, y, 0, true};
      plc_hmi_input_inject(&ev);
      return 0;
    }

    case WM_LBUTTONUP: {
      int16_t x = (int16_t)LOWORD(lp);
      int16_t y = (int16_t)HIWORD(lp);
      PlcHmiInputEvent ev = {PLC_HMI_INPUT_MOUSE, x, y, 0, false};
      plc_hmi_input_inject(&ev);
      return 0;
    }

    case WM_MOUSEMOVE: {
      if (wp & MK_LBUTTON) {
        int16_t x = (int16_t)LOWORD(lp);
        int16_t y = (int16_t)HIWORD(lp);
        PlcHmiInputEvent ev = {PLC_HMI_INPUT_MOUSE, x, y, 0, true};
        plc_hmi_input_inject(&ev);
      }
      return 0;
    }

    case WM_KEYDOWN: {
      uint16_t key = (uint16_t)wp;
      /* 映射方向键 */
      switch (key) {
        case VK_UP:    key = PLC_HMI_KEY_UP; break;
        case VK_DOWN:  key = PLC_HMI_KEY_DOWN; break;
        case VK_LEFT:  key = PLC_HMI_KEY_LEFT; break;
        case VK_RIGHT: key = PLC_HMI_KEY_RIGHT; break;
        case VK_RETURN: key = PLC_HMI_KEY_ENTER; break;
        case VK_ESCAPE: key = PLC_HMI_KEY_ESCAPE; break;
        case VK_F1:    key = PLC_HMI_KEY_F1; break;
        case VK_F12:   key = PLC_HMI_KEY_F12; break;
      }
      PlcHmiInputEvent ev = {PLC_HMI_INPUT_KEYBOARD, 0, 0, key, true};
      plc_hmi_input_inject(&ev);
      return 0;
    }

    case WM_KEYUP: {
      uint16_t key = (uint16_t)wp;
      PlcHmiInputEvent ev = {PLC_HMI_INPUT_KEYBOARD, 0, 0, key, false};
      plc_hmi_input_inject(&ev);
      return 0;
    }

    case WM_TIMER: {
      plc_hmi_update();
      InvalidateRect(hwnd, NULL, FALSE);
      return 0;
    }

    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;

    default:
      return DefWindowProc(hwnd, msg, wp, lp);
  }
}

/* ========== 显示驱动接口 ========== */

static int win32_init(uint16_t w, uint16_t h, uint8_t bpp)
{
  g_win_w = w;
  g_win_h = h;
  g_win_bpp = bpp;

  HINSTANCE hinst = GetModuleHandle(NULL);

  /* 注册窗口类 */
  WNDCLASSEX wc = {0};
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hinst;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszClassName = g_class_name;
  wc.hIcon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_APP_ICON));
  wc.hIconSm = wc.hIcon;
  RegisterClassEx(&wc);

  /* 计算窗口大小（含边框） */
  RECT rc = {0, 0, w, h};
  AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

  g_hwnd = CreateWindowEx(
    0, g_class_name, "Smart PLC HMI",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    rc.right - rc.left, rc.bottom - rc.top,
    NULL, NULL, hinst, NULL
  );

  if (!g_hwnd) return -1;

  g_hdc = GetDC(g_hwnd);
  create_dib(w, h);

  ShowWindow(g_hwnd, SW_SHOW);
  UpdateWindow(g_hwnd);

  /* 设置定时器 (16ms ≈ 60fps) */
  SetTimer(g_hwnd, 1, 16, NULL);

  g_driver_ready = true;
  return 0;
}

static void win32_deinit(void)
{
  if (g_hwnd) {
    KillTimer(g_hwnd, 1);
    DestroyWindow(g_hwnd);
    g_hwnd = NULL;
  }
  if (g_dib) {
    DeleteObject(g_dib);
    g_dib = NULL;
  }
  if (g_bg_brush) {
    DeleteObject(g_bg_brush);
    g_bg_brush = NULL;
  }
  g_driver_ready = false;
}

static void win32_flush(const void* fb, uint16_t w, uint16_t h, uint8_t bpp)
{
  (void)fb; (void)w; (void)h; (void)bpp;
  /* 刷新通过WM_PAINT完成，此处触发重绘 */
  if (g_hwnd) {
    InvalidateRect(g_hwnd, NULL, FALSE);
  }
}

static uint16_t win32_get_width(void)  { return g_win_w; }
static uint16_t win32_get_height(void) { return g_win_h; }

static const PlcHmiDriver g_win32_driver = {
  "win32-gdi",
  win32_init,
  win32_deinit,
  win32_flush,
  win32_get_width,
  win32_get_height
};

/* ========== 主入口 ========== */

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
  (void)hPrev; (void)lpCmd; (void)nShow;

  /* 初始化平台 */
  plc_platform_init();

  /* 注册Win32驱动 */
  plc_hmi_driver_register(PLC_HMI_DRV_WIN32, &g_win32_driver);

  /* 初始化HMI */
  PlcHmiConfig cfg = {0};
  cfg.screen_width = 800;
  cfg.screen_height = 480;
  cfg.bpp = 32;
  cfg.fps_target = 60;

  plc_hmi_init(&cfg);
  plc_hmi_driver_init(PLC_HMI_DRV_WIN32, 800, 480, 32);

  /* 加载演示界面 */
  plc_hmi_navigate("main");

  plc_hmi_start();

  /* 消息循环 */
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  plc_hmi_stop();
  win32_deinit();
  return (int)msg.wParam;
}

#endif /* _WIN32 */
