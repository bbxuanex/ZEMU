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
#include <cpu/difftest.h>
#include "../local-include/reg.h"

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc)
{
  // 1) PC
  if (cpu.pc != ref_r->pc)
  {
    // 建议打印: pc, cpu.pc, ref_r->pc
    printf("\n[DIFFTEST FAIL]\n");
    printf("  Description : PC mismatch\n");
    printf("  Happens at  : pc(arg)=0x%08x\n", (uint32_t)pc);
    printf("  DUT.pc      : 0x%08x\n", (uint32_t)cpu.pc);
    printf("  REF.pc      : 0x%08x\n", (uint32_t)ref_r->pc);
    printf("  XOR         : 0x%08x\n", (uint32_t)(cpu.pc ^ ref_r->pc));
    printf("  Next checks : nextpc update / branch-jump target / trap-return pc / pc update timing\n");
    return false;
  }

  // 2) GPRs
  int nr_gpr = (int)ARRLEN(cpu.gpr); // 16 or 32
  for (int i = 0; i < nr_gpr; i++)
  {
    if (cpu.gpr[i] != ref_r->gpr[i])
    {
      word_t dut = cpu.gpr[i];
      word_t ref = ref_r->gpr[i];
      word_t x = dut ^ ref;

      printf("\n[DIFFTEST FAIL]\n");
      printf("  Description : GPR mismatch\n");
      printf("  Happens at  : pc=0x%08x\n", (uint32_t)pc);
      printf("  Location    : x%d(%s)\n", i, reg_name(i));
      printf("  DUT         : 0x%08x\n", (uint32_t)dut);
      printf("  REF         : 0x%08x\n", (uint32_t)ref);
      printf("  XOR         : 0x%08x\n", (uint32_t)x);

      if (i == 0)
      {
        printf("  Hint        : x0 must stay 0 -> check writeback blocks writes to gpr[0]\n");
      }
      else if ((((uint32_t)x & 0xFFFF0000u) != 0) && (((uint32_t)x & 0x0000FFFFu) == 0))
      {
        printf("  Hint        : high-half differs -> suspect sign-ext / imm construction / sra\n");
      }
      else
      {
        printf("  Hint        : check writeback source(alu/load/pc+4), wb enable, rd select, mem read data\n");
      }

      printf("  Reference   : dut.pc=0x%08x ref.pc=0x%08x\n",
             (uint32_t)cpu.pc, (uint32_t)ref_r->pc);
      return false;
    }
  }
  return true;
}

void isa_difftest_attach()
{
}
