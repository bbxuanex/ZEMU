#include <common.h>
#include "syscall.h"

#ifdef CONFIG_STRACE
static const char *syscall_name[] = {
    [SYS_exit] = "exit",
    [SYS_yield] = "yield",
    [SYS_open] = "open",
    [SYS_read] = "read",
    [SYS_write] = "write",
    [SYS_kill] = "kill",
    [SYS_getpid] = "getpid",
    [SYS_close] = "close",
    [SYS_lseek] = "lseek",
    [SYS_brk] = "brk",
    [SYS_fstat] = "fstat",
    [SYS_time] = "time",
    [SYS_signal] = "signal",
    [SYS_execve] = "execve",
    [SYS_fork] = "fork",
    [SYS_link] = "link",
    [SYS_unlink] = "unlink",
    [SYS_wait] = "wait",
    [SYS_times] = "times",
    [SYS_gettimeofday] = "gettimeofday"};
#define NR_syscall sizeof(syscall_name) / sizeof(syscall_name[0])
const char *name = "Unknown Syscall_ID";
#endif
void do_syscall(Context *c)
{
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;
  switch (a[0])
  {
  case SYS_exit:
    halt(a[1]);
    break;
  case SYS_yield:
    yield();
    c->GPRx = 0;
    break;
  case SYS_write:
    const char *buf = (const char *)a[2];
    size_t len = (size_t)a[3];

    if (!buf && len > 0)
    {
      c->GPRx = -1;
      break;
    }
    if (a[1] == 1 || a[1] == 2)
    {
      for (int i = 0; i < len; ++i)
        putch(buf[i]);
      c->GPRx = len;
    }
    else
    {
      c->GPRx = -1;
    }
    break;
  case SYS_brk:
    c->GPRx = 0;
    Log("here");
    break;
  default:
    panic("Unhandled syscall ID = %d", a[0]);
  }
#ifdef CONFIG_STRACE
  if (a[0] < NR_syscall && syscall_name[a[0]])
    name = syscall_name[a[0]];
  Log("STRACE: Syscall_ID=%s, arg1=0x%x, arg2=0x%x, arg3=0x%x, ret=0x%x. \n", name, a[1], a[2], a[3], c->GPRx);
#endif
}