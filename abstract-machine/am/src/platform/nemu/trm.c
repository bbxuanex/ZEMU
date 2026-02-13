#include <am.h>
#include <nemu.h>
#include <klib.h>

extern char _heap_start;
int main(const char *args);

Area heap = RANGE(&_heap_start, PMEM_END);
static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS

void putch(char ch)
{
  outb(SERIAL_PORT, ch);
}

void halt(int code)
{
  nemu_trap(code);

  // should not reach here
  while (1)
    ;
}

void _trm_init()
{
  // 伪代码，需根据你现有的库函数调整
  printf("Heap Start: %x\n", heap.start);
  printf("Heap End:   %x\n", heap.end);

  int ret = main(mainargs);
  halt(ret);
}
