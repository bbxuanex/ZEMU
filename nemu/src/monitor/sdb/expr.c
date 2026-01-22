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
#include <memory/vaddr.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
enum
{
  TK_NOTYPE = 256,
  TK_EQ,    // ==
  TK_NEQ,   // !=
  TK_AND,   // &&
  TK_OR,    // ||
  TK_LE,    // <=
  TK_GE,    // >=
  TK_NOT,   // !
  TK_DEC,   // 十进制数
  TK_HEX,   // 十六进制数
  TK_REG,   // 寄存器
  TK_DEREF, // 指针解引用 *
  TK_NEG,   // 负号 -

  /* TODO: Add more token types */

};

static struct rule
{
  const char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE},          // spaces
    {"\\+", '+'},               // plus
    {"==", TK_EQ},              // equal
    {"!=", TK_NEQ},             // not equal
    {"&&", TK_AND},             // logical and
    {"\\|\\|", TK_OR},          // logical or
    {"<=", TK_LE},              //
    {">=", TK_GE},              //
    {"<", '<'},                 //
    {">", '>'},                 //
    {"!", TK_NOT},              // logical not
    {"-", '-'},                 // differ
    {"\\*", '*'},               // multiply
    {"/", '/'},                 // chuhao
    {"%", '%'},                 // mod
    {"\\(", '('},               // left
    {"\\)", ')'},               // right
    {"0x[0-9a-fA-F]+", TK_HEX}, // Hexadecimal
    {"[0-9]+", TK_DEC},         // Decimal
    {"\\$[a-zA-Z0-9]+", TK_REG},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

void init_regex()
{
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++)
  {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0)
    {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token
{
  int type;
  char str[32];
} Token;

static Token tokens[65536] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e)
{
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0')
  {
    for (i = 0; i < NR_REGEX; i++)
    {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;
        if (rules[i].token_type == TK_NOTYPE)
        {
          break;
        }

        if (substr_len >= 32)
        {
          panic("Buffer overflow: token is too long!");
        }

        Token *token = &tokens[nr_token];

        strncpy(token->str, substr_start, substr_len);

        token->str[substr_len] = '\0';
        token->type = rules[i].token_type;

        nr_token++;

        if (nr_token >= 65536)
        {
          panic("Error: Token buffer overflow!");
        }

        break;
      }
    }

    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  // 识别负号和指针解引用
  for (i = 0; i < nr_token; i++)
  {
    if (tokens[i].type == '-')
    {
      // 判断是负号还是减法
      // 负号的条件：前面不是数字、不是右括号、不是寄存器
      if (i == 0 ||
          (tokens[i - 1].type != TK_DEC &&
           tokens[i - 1].type != TK_HEX &&
           tokens[i - 1].type != TK_REG &&
           tokens[i - 1].type != ')'))
      {
        tokens[i].type = TK_NEG; // 标记为负号
      }
    }
    else if (tokens[i].type == '*')
    {
      // 判断是解引用还是乘法
      // 解引用的条件：前面不是数字、不是右括号、不是寄存器
      if (i == 0 ||
          (tokens[i - 1].type != TK_DEC &&
           tokens[i - 1].type != TK_HEX &&
           tokens[i - 1].type != TK_REG &&
           tokens[i - 1].type != ')'))
      {
        tokens[i].type = TK_DEREF; // 标记为指针解引用
      }
    }
  }

  return true;
}

static bool check_parentheses(int p, int q)
{
  if (tokens[p].type != '(' || tokens[q].type != ')')
  {
    return false;
  }

  int n = 0;
  for (int i = p; i < q; i++)
  {
    if (tokens[i].type == '(')
      n++;
    if (tokens[i].type == ')')
      n--;
    // Logic: 'cuz i ends at q while not reaching q,
    // so if ()couple exist only on two ends, the value of n shoud be 1!
    // it means that some other ')'appears before the ')'paired with '(' when n==0
    if (n == 0)
      return false;
  }

  if (tokens[q].type == ')')
    n--;

  return n == 0; // true means parentheses could be abandoned safely,,false? the opposite!
}

int find_main_operator(int p, int q)
{
  int op = -1;
  int paren = 0;

  // 第一轮：查找逻辑或 (||) - 最低优先级
  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == '(')
      paren++;
    else if (tokens[i].type == ')')
      paren--;

    if (paren == 0)
    {
      if (tokens[i].type == TK_OR)
      {
        op = i;
      }
    }
  }

  if (op != -1)
    return op;

  // 第二轮：查找逻辑与 (&&)
  paren = 0;
  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == '(')
      paren++;
    else if (tokens[i].type == ')')
      paren--;

    if (paren == 0)
    {
      if (tokens[i].type == TK_AND)
      {
        op = i;
      }
    }
  }

  if (op != -1)
    return op;

  // 第三轮：查找相等/不等运算符 (==, !=) - 优先级较低 (7)
  paren = 0;
  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == '(')
      paren++;
    else if (tokens[i].type == ')')
      paren--;

    if (paren == 0)
    {
      if (tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ)
      {
        op = i;
      }
    }
  }

  if (op != -1)
    return op; // 如果找到了 == 或 !=，它就是主运算符

  // 第四轮：查找关系运算符 (<, >, <=, >=) - 优先级稍高 (6)
  paren = 0;
  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == '(')
      paren++;
    else if (tokens[i].type == ')')
      paren--;

    if (paren == 0)
    {
      if (tokens[i].type == '<' || tokens[i].type == '>' ||
          tokens[i].type == TK_LE || tokens[i].type == TK_GE)
      {
        op = i;
      }
    }
  }

  if (op != -1)
    return op; // 如果没找到 ==，但找到了 < >，那它就是主运算符

  // 第五轮：查找加减运算符 (+, -)
  paren = 0;
  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == '(')
      paren++;
    else if (tokens[i].type == ')')
      paren--;

    if (paren == 0)
    {
      if (tokens[i].type == '+' || tokens[i].type == '-')
      {
        op = i;
      }
    }
  }

  if (op != -1)
    return op;

  // 第六轮：查找乘除取模运算符 (*, /, %)
  paren = 0;
  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == '(')
      paren++;
    else if (tokens[i].type == ')')
      paren--;

    if (paren == 0)
    {
      if (tokens[i].type == '*' || tokens[i].type == '/' || tokens[i].type == '%')
      {
        op = i;
      }
    }
  }

  return op;
}

static word_t eval(int p, int q)
{
  if (p > q)
  {
    return 0;
  }
  else if (p == q)
  {
    // Base case: 单个数字或寄存器
    if (tokens[p].type == TK_REG)
    {
      bool success;
      word_t val = isa_reg_str2val(tokens[p].str + 1, &success);
      if (!success)
      {
        printf("Unknown register: %s\n", tokens[p].str);
        return 0;
      }
      return val;
    }
    return strtoul(tokens[p].str, NULL, 0);
  }
  else if (check_parentheses(p, q) == true)
  {
    return eval(p + 1, q - 1);
  }
  else
  {
    int op = find_main_operator(p, q);

    if (op != -1)
    {
      word_t val1 = eval(p, op - 1);
      word_t val2 = eval(op + 1, q);

      switch (tokens[op].type)
      {
      case '+':
        return val1 + val2;
      case '-':
        return val1 - val2;
      case '*':
        return val1 * val2;
      case '/':
        if (val2 == 0)
        {
          printf("Error: Division by zero\n");
          return 0;
        }
        return (sword_t)val1 / (sword_t)val2;
      case '%':
        if (val2 == 0)
        {
          printf("Error: Modulo by zero\n");
          return 0;
        }
        return (sword_t)val1 % (sword_t)val2;
      case TK_EQ:
        return (sword_t)val1 == (sword_t)val2;
      case TK_NEQ:
        return (sword_t)val1 != (sword_t)val2;
      case '<':
        return (sword_t)val1 < (sword_t)val2;
      case '>':
        return (sword_t)val1 > (sword_t)val2;
      case TK_LE:
        return (sword_t)val1 <= (sword_t)val2;
      case TK_GE:
        return (sword_t)val1 >= (sword_t)val2;
      case TK_AND:
        return val1 && val2;
      case TK_OR:
        return val1 || val2;
      default:
        assert(0);
      }
    }
    // 3. 没找到双目运算符？那它肯定是一个单目运算表达式！
    else
    {
      // 检查开头的 token 类型
      int type = tokens[p].type;

      if (type == TK_NOT)
      {
        word_t val = eval(p + 1, q);
        return !val;
      }
      else if (tokens[p].type == TK_NEG)
      {
        word_t val = eval(p + 1, q);
        return -(sword_t)val; // 👈 加上 (sword_t)
      }

      else if (type == TK_DEREF)
      {
        word_t addr = eval(p + 1, q);
        return vaddr_read(addr, 4);
      }
      else
      {
        // 真的找不到了，或者是语法错误
        printf("[Error] Main operator not found in range [%d, %d]!\n", p, q);
        return 0;
      }
    }

    return 0;
  }
}

word_t expr(char *e, bool *success)
{
  if (!make_token(e))
  {
    *success = false;
    return 0;
  }

  /* Start evaluation from the first to the last token */
  *success = true;
  return eval(0, nr_token - 1);
}