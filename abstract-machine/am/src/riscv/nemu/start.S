.section entry, "ax"
.globl _start
.type _start, @function

_start:
  mv s0, zero
  la gp, __global_pointer$
  la sp, _stack_pointer
  call _trm_init

.size _start, . - _start
