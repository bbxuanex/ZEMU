.section entry, "ax"
.globl _start
.type _start, @function

_start:
  mv s0, zero
  li sp, 0x88000000
  call _trm_init

.size _start, . - _start
