#define PLATFORM_WIN32

#include "plc_platform.h"
#include "plc_io.h"
#include "plc_comm.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <process.h>
#include <malloc.h>
#if defined(_M_IX86) || defined(__i386__)
#include <xmmintrin.h>
#if !defined(_MSC_VER)
/* MinGW: _inp/_outp 在 libmsvcrt 中但无头文件声明 */
extern int _inp(unsigned short port);
extern void _outp(unsigned short port, int val);
#endif
#endif

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

/* ========== 高精度计时 ========== */

static LARGE_INTEGER g_freq;
static double g_freqInvUs;
static double g_freqInvMs;
static BOOL g_highResTimer = FALSE;

/* 多媒体定时器分辨率 */
static UINT g_mmTimerRes = 0;

/* ========== 临界区 ========== */

static CRITICAL_SECTION g_criticalSection;
static BOOL g_csInit = FALSE;

/* ========== 伺服定时器 (实时扩展) ========== */

typedef struct {
  HANDLE timerQueue;
  HANDLE timer;
  void (*callback)(void*);
  void *context;
  DWORD periodMs;
  volatile BOOL running;
} WinRTTimer;

static WinRTTimer g_servoTimer;

/* ========== 实时线程管理 ========== */

typedef struct {
  HANDLE handle;
  DWORD id;
  char name[32];
  int priority;
  uint32_t cpuMask;
  BOOL created;
} WinRTThread;

#define WIN_MAX_RT_THREADS 16
static WinRTThread g_rtThreads[WIN_MAX_RT_THREADS];
static int g_rtThreadCount = 0;

/* ========== 平台初始化 ========== */

void plc_platform_init(void)
{
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  /* 高精度计时器初始化 */
  g_highResTimer = QueryPerformanceFrequency(&g_freq);
  if (g_highResTimer && g_freq.QuadPart > 0) {
    g_freqInvUs = 1000000.0 / (double)g_freq.QuadPart;
    g_freqInvMs = 1000.0 / (double)g_freq.QuadPart;
  } else {
    g_freqInvUs = 0.001; /* 回退到 1μs = 1 计数器单位 */
    g_freqInvMs = 1.0;
  }

  /* 初始化临界区 */
  InitializeCriticalSection(&g_criticalSection);
  g_csInit = TRUE;

  /* 设置高分辨率定时器 (1ms 精度) */
  TIMECAPS tc;
  if (timeGetDevCaps(&tc, sizeof(tc)) == TIMERR_NOERROR) {
    g_mmTimerRes = max(tc.wPeriodMin, 1);
    timeBeginPeriod(g_mmTimerRes);
  }

  /* 提升当前进程优先级 */
  HANDLE hProcess = GetCurrentProcess();
  SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);

  /* 初始化伺服定时器结构 */
  memset(&g_servoTimer, 0, sizeof(g_servoTimer));

  /* 初始化实时线程表 */
  memset(g_rtThreads, 0, sizeof(g_rtThreads));
  g_rtThreadCount = 0;

  plc_platform_log(PLC_LOG_INFO, "Win32 实时平台初始化完成 (timer=%dμs)",
    (int)(g_freqInvUs * g_freq.QuadPart));
}

/* ========== 时间函数 ========== */

uint32_t plc_platform_tick_ms(void)
{
  if (g_highResTimer) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (uint32_t)((double)now.QuadPart * g_freqInvMs);
  }
  return timeGetTime();
}

uint64_t plc_platform_tick_us(void)
{
  if (g_highResTimer) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (uint64_t)((double)now.QuadPart * g_freqInvUs);
  }
  return (uint64_t)timeGetTime() * 1000ULL;
}

void plc_platform_delay_ms(uint32_t ms)
{
  if (ms <= 1) {
    /* 短延时使用 busy-wait 保证精度 */
    uint64_t target = plc_platform_tick_us() + (uint64_t)ms * 1000ULL;
    while (plc_platform_tick_us() < target) { Sleep(0); }
  } else {
    Sleep(ms);
  }
}

void plc_platform_delay_us(uint32_t us)
{
  if (us >= 2000) {
    Sleep(us / 1000);
  } else {
    uint64_t target = plc_platform_tick_us() + us;
    while (plc_platform_tick_us() < target) {
      _mm_pause();
    }
  }
}

/* ========== 临界区 ========== */

void plc_platform_critical_enter(void)
{
  if (g_csInit) EnterCriticalSection(&g_criticalSection);
}

void plc_platform_critical_exit(void)
{
  if (g_csInit) LeaveCriticalSection(&g_criticalSection);
}

/* ========== 内存管理 ========== */

void* plc_platform_malloc(size_t size)
{
  return malloc(size);
}

void plc_platform_free(void* ptr)
{
  free(ptr);
}

/* ========== 日志 ========== */

static const char* g_logLevelNames[] = {
  "ERROR", "WARN ", "INFO ", "DEBUG", "TRACE"
};

void plc_platform_log(uint8_t level, const char* fmt, ...)
{
  if (level > PLC_LOG_DEBUG) return;

  SYSTEMTIME st;
  GetLocalTime(&st);

  char prefix[64];
  snprintf(prefix, sizeof(prefix),
    "[%04d-%02d-%02d %02d:%02d:%02d.%03d] [%s] ",
    st.wYear, st.wMonth, st.wDay,
    st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
    g_logLevelNames[level]);

  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  HANDLE hOut = GetStdHandle(STD_ERROR_HANDLE);
  DWORD written;
  WriteFile(hOut, prefix, (DWORD)strlen(prefix), &written, NULL);
  WriteFile(hOut, buf, (DWORD)strlen(buf), &written, NULL);
  WriteFile(hOut, "\n", 1, &written, NULL);
}

/* ========== 实时定时器 (Servo 循环) ========== */

static void CALLBACK servoTimerCallback(PVOID param, BOOLEAN timerOrWait)
{
  (void)timerOrWait;
  WinRTTimer *rt = (WinRTTimer *)param;
  if (rt->running && rt->callback) {
    rt->callback(rt->context);
  }
}

int plc_win_rt_createServo(uint32_t periodUs, void (*callback)(void*), void *context)
{
  if (!callback) return -1;

  g_servoTimer.callback = callback;
  g_servoTimer.context = context;
  g_servoTimer.periodMs = periodUs / 1000;
  if (g_servoTimer.periodMs < 1) g_servoTimer.periodMs = 1;
  g_servoTimer.running = FALSE;

  g_servoTimer.timerQueue = CreateTimerQueue();
  if (!g_servoTimer.timerQueue) return -1;

  return 0;
}

int plc_win_rt_startServo(void)
{
  if (!g_servoTimer.timerQueue) return -1;
  if (g_servoTimer.running) return 0;

  g_servoTimer.running = TRUE;

  BOOL ok = CreateTimerQueueTimer(
    &g_servoTimer.timer,
    g_servoTimer.timerQueue,
    servoTimerCallback,
    &g_servoTimer,
    g_servoTimer.periodMs,
    g_servoTimer.periodMs,
    WT_EXECUTEINTIMERTHREAD | WT_EXECUTEINPERSISTENTTHREAD);

  if (!ok) {
    g_servoTimer.running = FALSE;
    return -1;
  }

  return 0;
}

int plc_win_rt_stopServo(void)
{
  if (!g_servoTimer.timerQueue) return -1;
  if (!g_servoTimer.running) return 0;

  g_servoTimer.running = FALSE;

  if (g_servoTimer.timer) {
    DeleteTimerQueueTimer(g_servoTimer.timerQueue, g_servoTimer.timer, INVALID_HANDLE_VALUE);
    g_servoTimer.timer = NULL;
  }

  return 0;
}

void plc_win_rt_destroyServo(void)
{
  plc_win_rt_stopServo();

  if (g_servoTimer.timerQueue) {
    DeleteTimerQueueEx(g_servoTimer.timerQueue, INVALID_HANDLE_VALUE);
    g_servoTimer.timerQueue = NULL;
  }

  memset(&g_servoTimer, 0, sizeof(g_servoTimer));
}

/* ========== 实时线程 API ========== */

static unsigned int __stdcall rtThreadEntry(void *arg)
{
  WinRTThread *rt = (WinRTThread *)arg;

  /* 设置线程优先级 */
  HANDLE hThread = GetCurrentThread();
  int prio = THREAD_PRIORITY_TIME_CRITICAL;
  switch (rt->priority) {
    case 1:  prio = THREAD_PRIORITY_LOWEST; break;
    case 2:  prio = THREAD_PRIORITY_BELOW_NORMAL; break;
    case 3:  prio = THREAD_PRIORITY_NORMAL; break;
    case 4:  prio = THREAD_PRIORITY_ABOVE_NORMAL; break;
    case 5:  prio = THREAD_PRIORITY_HIGHEST; break;
    default: prio = THREAD_PRIORITY_TIME_CRITICAL; break;
  }
  SetThreadPriority(hThread, prio);

  /* 设置 CPU 亲和性 */
  if (rt->cpuMask > 0) {
    SetThreadAffinityMask(hThread, (DWORD_PTR)rt->cpuMask);
  }

  return 0;
}

int plc_win_rt_createThread(const char *name, int priority,
                            uint32_t cpuMask, void **outHandle)
{
  if (g_rtThreadCount >= WIN_MAX_RT_THREADS) return -1;

  WinRTThread *rt = &g_rtThreads[g_rtThreadCount];
  memset(rt, 0, sizeof(WinRTThread));
  strncpy(rt->name, name ? name : "RT-Thread", sizeof(rt->name) - 1);
  rt->priority = priority;
  rt->cpuMask = cpuMask;

  rt->handle = (HANDLE)_beginthreadex(
    NULL, 0, rtThreadEntry, rt, CREATE_SUSPENDED, (unsigned *)&rt->id);

  if (!rt->handle) return -1;

  rt->created = TRUE;

  if (outHandle) *outHandle = rt->handle;
  g_rtThreadCount++;

  return 0;
}

int plc_win_rt_resumeThread(void *hThread)
{
  return ResumeThread((HANDLE)hThread) != (DWORD)-1 ? 0 : -1;
}

void plc_win_rt_sleepUs(uint32_t us)
{
  /* 使用 waitable timer 实现高精度睡眠 */
  HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
  if (timer) {
    LARGE_INTEGER due;
    due.QuadPart = -(LONGLONG)(us * 10LL); /* 100ns 单位 */
    SetWaitableTimer(timer, &due, 0, NULL, NULL, FALSE);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
  }
}

/* ========== Windows I/O 硬件访问 ========== */

/* I/O 端口访问: 仅 x86 (32-bit) 支持用户态端口 I/O
 * x64 需要内核驱动 (如 WinRing0, inpout32) */
#if defined(_M_IX86) || defined(__i386__)
static BOOL g_ioPrivilege = FALSE;

static void enableIoPort(void)
{
  static HANDLE hDriver = INVALID_HANDLE_VALUE;
  if (hDriver != INVALID_HANDLE_VALUE) return;

  /* 尝试打开驱动获取 I/O 权限 (需要 GiveIO/PortTalk 类驱动) */
  hDriver = CreateFile("\\\\.\\GiveIO", GENERIC_READ, 0, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hDriver != INVALID_HANDLE_VALUE) {
    g_ioPrivilege = TRUE;
    return;
  }

  /* 备选: WinRing0 */
  hDriver = CreateFile("\\\\.\\WinRing0_1_2_0", GENERIC_READ | GENERIC_WRITE, 0,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hDriver != INVALID_HANDLE_VALUE) {
    g_ioPrivilege = TRUE;
  }
}

static inline uint8_t inb_port(uint16_t port)
{
  if (!g_ioPrivilege) return 0;
#ifdef _MSC_VER
  return _inp((unsigned short)port);
#else
  return (uint8_t)_inp((unsigned short)port);
#endif
}

static inline void outb_port(uint16_t port, uint8_t val)
{
  if (!g_ioPrivilege) return;
#ifdef _MSC_VER
  _outp((unsigned short)port, val);
#else
  _outp((unsigned short)port, (unsigned short)val);
#endif
}
#else
/* x64 Windows 不支持用户态 I/O 端口访问 */
static void enableIoPort(void) {}
static inline uint8_t inb_port(uint16_t port) { (void)port; return 0; }
static inline void outb_port(uint16_t port, uint8_t val) { (void)port; (void)val; }
#endif

/* ========== I/O HAL (Windows 实现) ========== */

#if !defined(PLATFORM_WIN32_IOTYPE)
#define PLATFORM_WIN32_IOTYPE IO_TYPE_DI
#endif

int32_t plc_hal_read_input(uint32_t physical_addr, IoType type)
{
  switch (type) {
    case IO_TYPE_DI:
      return (int32_t)inb_port((uint16_t)physical_addr);

    case IO_TYPE_AI: {
      uint16_t port = (uint16_t)physical_addr;
      uint32_t val = inb_port(port);
      val |= (uint32_t)inb_port(port + 1) << 8;
      return (int32_t)(val & 0x0FFF);
    }

    case IO_TYPE_ENCODER:
    case IO_TYPE_COUNTER: {
      uint16_t port = (uint16_t)physical_addr;
      uint32_t val = inb_port(port);
      val |= (uint32_t)inb_port(port + 1) << 8;
      val |= (uint32_t)inb_port(port + 2) << 16;
      val |= (uint32_t)inb_port(port + 3) << 24;
      return (int32_t)val;
    }

    default:
      return 0;
  }
}

void plc_hal_write_output(uint32_t physical_addr, IoType type, int32_t value)
{
  switch (type) {
    case IO_TYPE_DO:
      outb_port((uint16_t)physical_addr, value ? 0x01 : 0x00);
      break;

    case IO_TYPE_AO: {
      uint16_t port = (uint16_t)physical_addr;
      outb_port(port,     (uint8_t)(value & 0xFF));
      outb_port(port + 1, (uint8_t)((value >> 8) & 0xFF));
      break;
    }

    case IO_TYPE_PWM:
      /* PWM 输出通过硬件寄存器或虚拟设备 */
      outb_port((uint16_t)physical_addr, (uint8_t)value);
      break;

    default:
      break;
  }
}

/* ========== TCP 通信 (Winsock) ========== */

static int g_winsockInit = 0;

static int ensureWinsock(void)
{
  if (!g_winsockInit) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
    g_winsockInit = 1;
  }
  return 0;
}

int plc_hal_tcp_connect(const char* host, uint16_t port, uint32_t timeout_ms)
{
  if (ensureWinsock() < 0) return -1;

  SOCKET fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd == INVALID_SOCKET) return -1;

  /* 非阻塞连接 */
  u_long mode = 1;
  ioctlsocket(fd, FIONBIO, &mode);

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(host);

  if (addr.sin_addr.s_addr == INADDR_NONE) {
    struct hostent *he = gethostbyname(host);
    if (!he) { closesocket(fd); return -1; }
    memcpy(&addr.sin_addr, he->h_addr, he->h_length);
  }

  connect(fd, (struct sockaddr*)&addr, sizeof(addr));

  /* 等待连接 */
  fd_set writeSet;
  FD_ZERO(&writeSet);
  FD_SET(fd, &writeSet);
  struct timeval tv;
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;

  int ret = select((int)(fd + 1), NULL, &writeSet, NULL, &tv);
  if (ret <= 0) {
    closesocket(fd);
    return -1;
  }

  mode = 0;
  ioctlsocket(fd, FIONBIO, &mode);

  int opt = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(opt));

  return (int)fd;
}

void plc_hal_tcp_close(int fd)
{
  if (fd >= 0) closesocket((SOCKET)fd);
}

int plc_hal_tcp_send(int fd, const uint8_t* data, uint32_t len)
{
  uint32_t sent = 0;
  while (sent < len) {
    int n = send((SOCKET)fd, (const char*)(data + sent), (int)(len - sent), 0);
    if (n <= 0) return -1;
    sent += (uint32_t)n;
  }
  return (int)sent;
}

int plc_hal_tcp_recv(int fd, uint8_t* data, uint32_t max_len, uint32_t timeout_ms)
{
  if (timeout_ms > 0) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET((SOCKET)fd, &readSet);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (select((int)(fd + 1), &readSet, NULL, NULL, &tv) <= 0) {
      return 0;
    }
  }

  int n = recv((SOCKET)fd, (char*)data, (int)max_len, 0);
  return (n >= 0) ? n : -1;
}

/* ========== 串口通信 (Win32 COM) ========== */

int plc_hal_serial_open(const char* port_name, uint32_t baud_rate,
                         uint8_t data_bits, uint8_t stop_bits, uint8_t parity)
{
  char path[32];
  snprintf(path, sizeof(path), "\\\\.\\%s", port_name);

  HANDLE hCom = CreateFileA(path,
    GENERIC_READ | GENERIC_WRITE,
    0, NULL, OPEN_EXISTING, 0, NULL);

  if (hCom == INVALID_HANDLE_VALUE) return -1;

  DCB dcb;
  memset(&dcb, 0, sizeof(dcb));
  dcb.DCBlength = sizeof(DCB);

  if (!GetCommState(hCom, &dcb)) {
    CloseHandle(hCom);
    return -1;
  }

  dcb.BaudRate = baud_rate;
  dcb.fBinary = TRUE;
  dcb.fParity = FALSE;
  dcb.fOutxCtsFlow = FALSE;
  dcb.fOutxDsrFlow = FALSE;
  dcb.fDtrControl = DTR_CONTROL_DISABLE;
  dcb.fRtsControl = RTS_CONTROL_DISABLE;
  dcb.fInX = FALSE;
  dcb.fOutX = FALSE;
  dcb.fErrorChar = FALSE;
  dcb.fNull = FALSE;

  switch (data_bits) {
    case 5: dcb.ByteSize = 5; break;
    case 6: dcb.ByteSize = 6; break;
    case 7: dcb.ByteSize = 7; break;
    default: dcb.ByteSize = 8; break;
  }

  dcb.StopBits = (stop_bits >= 2) ? TWOSTOPBITS : ONESTOPBIT;

  switch (parity) {
    case 1: dcb.Parity = EVENPARITY; dcb.fParity = TRUE; break;
    case 2: dcb.Parity = ODDPARITY;  dcb.fParity = TRUE; break;
    default: dcb.Parity = NOPARITY;  break;
  }

  if (!SetCommState(hCom, &dcb)) {
    CloseHandle(hCom);
    return -1;
  }

  COMMTIMEOUTS timeouts;
  timeouts.ReadIntervalTimeout = 1;
  timeouts.ReadTotalTimeoutMultiplier = 0;
  timeouts.ReadTotalTimeoutConstant = 100;
  timeouts.WriteTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = 100;
  SetCommTimeouts(hCom, &timeouts);

  return (int)hCom;
}

void plc_hal_serial_close(int fd)
{
  if (fd >= 0) CloseHandle((HANDLE)fd);
}

int plc_hal_serial_send(int fd, const uint8_t* data, uint32_t len)
{
  HANDLE hCom = (HANDLE)fd;
  DWORD written = 0;
  if (!WriteFile(hCom, data, len, &written, NULL)) return -1;
  FlushFileBuffers(hCom);
  return (int)written;
}

int plc_hal_serial_recv(int fd, uint8_t* data, uint32_t max_len, uint32_t timeout_ms)
{
  HANDLE hCom = (HANDLE)fd;

  if (timeout_ms > 0) {
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = timeout_ms;
    SetCommTimeouts(hCom, &timeouts);
  }

  DWORD read = 0;
  if (!ReadFile(hCom, data, max_len, &read, NULL)) return -1;
  return (int)read;
}

/* ========== GPIO HAL 模拟（Win32 使用内存模拟 GPIO） ========== */

static uint8_t g_gpio_sim[256]; /* 虚拟 GPIO 寄存器 */

int32_t plc_hal_gpio_read(uint32_t addr)
{
  if (addr < 256) return (g_gpio_sim[addr] != 0) ? 1 : 0;
  return 0;
}

void plc_hal_gpio_write(uint32_t addr, int32_t value)
{
  if (addr < 256) g_gpio_sim[addr] = (uint8_t)(value ? 1 : 0);
}

void plc_hal_gpio_toggle(uint32_t addr)
{
  if (addr < 256) g_gpio_sim[addr] = g_gpio_sim[addr] ? 0 : 1;
}

void plc_hal_step_pulse(uint32_t step_addr, uint32_t dir_addr, int32_t dir)
{
  /* Win32 模拟：设置方向 + 翻转 STEP 引脚 */
  if (step_addr < 256 && dir_addr < 256) {
    g_gpio_sim[dir_addr] = (uint8_t)(dir ? 1 : 0);
    g_gpio_sim[step_addr] = 1;
    /* 模拟 2μs 脉冲宽度 */
    g_gpio_sim[step_addr] = 0;
  }
}
