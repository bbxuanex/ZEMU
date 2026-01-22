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
  int loop = 10000;
  if (argc > 1)
    sscanf(argv[1], "%d", &loop);

  int count = 0;
  while (count < loop)
  {
    // 1. 创建临时文件
    FILE *fp = fopen("/tmp/.code.c", "w");
    fprintf(fp, "#include <stdio.h>\n");
    fprintf(fp, "int main() { unsigned long result;\n");

    // 2. 批量生成：一次生成一批（比如500条或剩余需要的数量）
    //    如果这批里有除0，程序会崩，我们只能拿到崩之前的输出。
    //    没关系，崩了我们就再循环一次，补齐剩下的。
    int batch_size = loop - count;
    if (batch_size > 500)
      batch_size = 500;

    for (int i = 0; i < batch_size; i++)
    {
      gen_rand_expr(0);
      // 关键点：每打印一条，必须 fflush，确保如果下一条崩了，这一条已经输出了
      fprintf(fp, "result = %s; ", buf);
      fprintf(fp, "printf(\"%%lu %%s\\n\", result, \"%s\"); ", buf);
      fprintf(fp, "fflush(stdout);\n");
    }

    fprintf(fp, "return 0; }\n");
    fclose(fp);

    // 3. 编译这个临时文件
    // 使用 -w 关闭所有警告，让它安静点
    int ret = system("gcc /tmp/.code.c -o /tmp/.expr -w");
    if (ret != 0)
      continue; // 编译失败（极少见），重试

    // 4. 运行并通过管道读取输出
    // popen 会启动一个子进程运行程序
    FILE *in = popen("/tmp/.expr", "r");
    if (!in)
      continue;

    char line[4096];
    // 5. 逐行读取结果
    while (fgets(line, sizeof(line), in))
    {
      printf("%s", line); // 输出给 sdb.c 使用
      count++;
      if (count >= loop)
        break;
    }

    pclose(in);
  }

  return 0;
}
