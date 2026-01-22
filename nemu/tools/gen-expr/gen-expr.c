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

static uint32_t choose(uint32_t n)
{
  return rand() % n; // return random value?shuimushi
}

static void gen_rand_expr(int length)
{
  if (strlen(buf) > 500)
  {
    char num_buf[32];
    sprintf(num_buf, "%d", choose(100)); // sprintf?what is that?shuimushi
    strcat(buf, num_buf);
    return;
  }

  switch (choose(4))
  {
  case 0:
  { // number generate
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
  // 1. 初始化随机种子
  srand(time(0));

  int loop = 10000;
  if (argc > 1)
  {
    sscanf(argv[1], "%d", &loop);
  }

  int i = 0;
  while (i < loop)
  {
    // 2. 生成随机表达式
    buf[0] = '\0';
    gen_rand_expr(0);

    // 过滤太短的
    if (strlen(buf) < 3)
    {
      continue;
    }

    // 3. 为【这一条】表达式单独生成一个 C 程序
    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);

    fprintf(fp, "#include <stdio.h>\n");
    fprintf(fp, "#include <stdint.h>\n"); // 引入 uint32_t
    fprintf(fp, "int main() {\n");

    // 强制使用 uint32_t 计算，确保和 NEMU 行为一致
    fprintf(fp, "  uint32_t result = %s;\n", buf);
    fprintf(fp, "  printf(\"%%u %%s\\n\", result, \"%s\");\n", buf);

    fprintf(fp, "  return 0;\n");
    fprintf(fp, "}\n");
    fclose(fp);

    // 4. 编译这个小程序
    int ret = system("gcc -O2 /tmp/.code.c -o /tmp/.expr -w");
    if (ret != 0)
    {
      continue; // 编译失败（极少见），重试
    }

    // 5. 运行这个小程序
    // 如果表达式里有除0，这里会返回非0值（崩溃）
    // 2> /dev/null 的意思是：把错误信息（stderr）丢掉，不显示在屏幕上
    ret = system("/tmp/.expr 2> /dev/null");

    if (ret == 0)
    {
      i++;
    }
  }

  return 0;
}
