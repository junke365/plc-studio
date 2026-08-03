#include <sys/stat.h>
#include <sys/times.h>
#include <errno.h>
#undef errno
extern int errno;

void _exit(int status)
{
  (void)status;
  while (1) {}
}

int _close(int file)
{
  (void)file;
  return -1;
}

int _fstat(int file, struct stat* st)
{
  (void)file;
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int file)
{
  (void)file;
  return 1;
}

int _lseek(int file, int ptr, int dir)
{
  (void)file; (void)ptr; (void)dir;
  return 0;
}

int _read(int file, char* ptr, int len)
{
  (void)file; (void)ptr; (void)len;
  return 0;
}

__attribute__((weak)) int _write(int file, char* ptr, int len)
{
  (void)file; (void)ptr;
  return len;
}

caddr_t _sbrk(int incr)
{
  extern char _heap_start;
  extern char _heap_end;
  static char* heap_end = &_heap_start;
  char* prev_heap_end = heap_end;
  if (heap_end + incr > &_heap_end) {
    return (caddr_t)-1;
  }
  heap_end += incr;
  return (caddr_t)prev_heap_end;
}

int _kill(int pid, int sig)
{
  (void)pid; (void)sig;
  errno = EINVAL;
  return -1;
}

int _getpid(void)
{
  return 1;
}

int _times(struct tms* buf)
{
  (void)buf;
  return -1;
}
