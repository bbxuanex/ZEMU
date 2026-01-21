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

// 辅助函数：生成 [0, n-1] 的随机数
static uint32_t choose(uint32_t n)
{
  return rand() % n;
}

// 递归生成随机表达式
static void gen_rand_expr(int length)
{
  // 1. 长度限制
  if (strlen(buf) > 500)
  {
    char num_buf[32];
    sprintf(num_buf, "%d", choose(100));
    strcat(buf, num_buf);
    return;
  }

  // 2. 随机选择生成类型
  // 0: 数字
  // 1: 括号 ( expr )
  // 2: 单目运算 ( !expr, -expr )
  // 3: 二元运算 ( expr OP expr )
  switch (choose(4))
  {
  case 0:
  { // 生成数字
    char num_buf[32];
    // 20% 概率生成十六进制
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
  { // 生成单目运算 (! 或 -)
    // 注意：不生成 * (解引用)，因为随机地址会导致 GCC 崩溃
    if (choose(2) == 0)
    {
      strcat(buf, "-"); // 负号
    }
    else
    {
      strcat(buf, "!"); // 逻辑非
    }
    // 为了避免连续符号解析困难（如 --1），加个括号保险，或者直接接表达式
    // 这里简单处理，直接递归，依靠空格插入来分隔
    strcat(buf, "("); // 加括号比较稳妥，例如 -(1+2)
    gen_rand_expr(length + 1);
    strcat(buf, ")");
    break;
  }
  default:
  { // 生成二元运算
    gen_rand_expr(length + 1);

    // 你的 expr.c 支持的所有二元运算符
    char *op;
    switch (choose(13))
    {
    case 0:
      op = "+";
      break;
    case 1:
      op = "-";
      break;
    case 2:
      op = "*";
      break;
    case 3:
      op = "/";
      break;
    case 4:
      op = "%";
      break; // 取模
    case 5:
      op = "==";
      break; // 等于
    case 6:
      op = "!=";
      break; // 不等于
    case 7:
      op = "&&";
      break; // 逻辑与
    case 8:
      op = "||";
      break; // 逻辑或
    case 9:
      op = "<";
      break;
    case 10:
      op = ">";
      break;
    case 11:
      op = "<=";
      break;
    case 12:
      op = ">=";
      break;
    }

    strcat(buf, op);
    gen_rand_expr(length + 1);
    break;
  }
  }
}

// 随机插入空格
static void insert_random_spaces()
{
  char temp[BUF_SIZE];
  int j = 0;
  for (int i = 0; buf[i] != '\0'; i++)
  {
    temp[j++] = buf[i];
    if (choose(10) < 3)
    { // 30% 概率插入空格
      temp[j++] = ' ';
    }
  }
  temp[j] = '\0';
  if (j < BUF_SIZE - 1)
    strcpy(buf, temp);
}

int main(int argc, char *argv[])
{
  int seed = time(0);
  srand(seed);
  int loop = 100; // 默认生成数量
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

    insert_random_spaces();

    // 构造 C 代码
    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    // 编译 (-w 关闭警告)
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