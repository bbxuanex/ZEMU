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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

// 缓冲区大小
#define BUF_SIZE 65536

static char buf[BUF_SIZE];
static char code_buf[BUF_SIZE + 128];
static char *code_format =
    "#include <stdio.h>\n"
    "int main() { "
    "  unsigned long result = %s; "
    "  printf(\"%%lu\", result); "
    "  return 0; "
    "}";

static uint32_t choose(uint32_t n)
{
  return rand() % n;
}

static void gen_rand_expr(int length)
{
  if (strlen(buf) > 500)
  {
    char num_buf[32];
    sprintf(num_buf, "%d", choose(100));
    strcat(buf, num_buf);
    return;
  }

  switch (choose(4)) // 从这里往下看，可以看到choose4是有4个case与Default构成的，为什么呢，因为对近无穷可能的数来说，取模后得0，1，2，3（就是Default）的概率是等的
                     // 同样的结构可以在双目运算符的构造中看到

  {
  case 0:
  { // 生成数字
    char num_buf[32];
    // 减少十六进制生成的概率，防止因为无符号数溢出问题太复杂
    if (choose(5) == 0)
    {
      sprintf(num_buf, "0x%x", choose(1000) + 1);
    }
    else
    {
      sprintf(num_buf, "%d", choose(1000) + 1);
    }
    strcat(buf, num_buf);
    break;
  }
  case 1: // 生成 ( expr )
    strcat(buf, "(");
    gen_rand_expr(length + 1);
    strcat(buf, ")");
    break;
  case 2:
  { // 生成单目运算
    if (choose(2) == 0)
    {
      strcat(buf, " -"); // 前面加个空格防止粘连
    }
    else
    {
      strcat(buf, " !");
    }
    strcat(buf, "(");
    gen_rand_expr(length + 1);
    strcat(buf, ")");
    break;
  }
  default:
  { // 生成二元运算
    gen_rand_expr(length + 1);

    char *op;
    // 在运算符两边直接加上空格，确保不会粘连，也不用后续随机插空格了
    switch (choose(13))
    {
    case 0:
      op = " + ";
      break;
    case 1:
      op = " - ";
      break;
    case 2:
      op = " * ";
      break;
    case 3:
      op = " / ";
      break;
    case 4:
      op = " % ";
      break;
    case 5:
      op = " == ";
      break;
    case 6:
      op = " != ";
      break;
    case 7:
      op = " && ";
      break;
    case 8:
      op = " || ";
      break;
    case 9:
      op = " < ";
      break;
    case 10:
      op = " > ";
      break;
    case 11:
      op = " <= ";
      break;
    case 12:
      op = " >= ";
      break;
    }

    strcat(buf, op);
    gen_rand_expr(length + 1);
    break;
  }
  }
}

int main(int argc, char *argv[])
{
  int seed = time(0);
  srand(seed);
  int loop = 10000;
  if (argc > 1)
  {
    sscanf(argv[1], "%d", &loop);
  }

  int i;
  for (i = 0; i < loop; i++)
  {
    buf[0] = '\0';

    gen_rand_expr(0);

    if (strlen(buf) < 3)
    {
      i--;
      continue;
    }

    // 移除了 insert_random_spaces() 调用，防止破坏双字符运算符

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr -w");
    if (ret != 0)
    {
      i--;
      continue;
    }

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    unsigned long result;
    if (fscanf(fp, "%lu", &result) != 1)
    {
      pclose(fp);
      i--;
      continue;
    }
    pclose(fp);

    printf("%lu %s\n", result, buf);
  }
  return 0;
}