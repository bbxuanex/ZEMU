/***************************************************************************************
 * Copyright (c) 2014-2024 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <isa.h>

word_t isa_raise_intr(word_t NO, vaddr_t epc)
{
/* TODO: Trigger an interrupt/exception with ``NO''.
 * Then return the address of the interrupt/exception vector.
 */
#ifdef CONFIG_ETRACE
  char *exception = "!--- Unknown Type ---!";
  word_t if_intr = NO & 0x80000000;
  word_t cause = NO & 0x7fffffff;
  if (if_intr)
  {
    switch (cause)
    {
    case 1:
      exception = "Supervisor software interrupt";
      break;

      // TODO: Add more interrupt case.
    }
  }
  else
  {
    switch (cause)
    {
    case 0:
      exception = "Instruction address misaligned";
      break;
    case 1:
      exception = "Instruction access fault";
      break;
    case 2:
      exception = "Illegal instruction";
      break;
    case 3:
      exception = "Breakpoint";
      break;
    case 4:
      exception = "Load address misaligned";
      break;
    case 5:
      exception = "Load access fault";
      break;
    case 6:
      exception = "Store/AMO address misaligned";
      break;
    case 7:
      exception = "Store/AMO access fault";
      break;
    case 8:
      exception = "Environment call from U-mode";
      break;
    case 9:
      exception = "Environment call from S-mode";
      break;
    case 11:
      exception = "Environment call from M-mode";
      break;
    case 12:
      exception = "Instruction page fault";
      break;
    case 13:
      exception = "Load page fault";
      break;
    case 15:
      exception = "Store/AMO page fault";
      break;
    }
  }
  Log("ETRACE: Exception at PC = 0x%08x, cause = %d [%s]\n", epc, cause, exception);
#endif
  cpu.mcause = NO;
  cpu.mepc = epc;
  return cpu.mtvec;
}

word_t isa_query_intr()
{
  return INTR_EMPTY;
}
