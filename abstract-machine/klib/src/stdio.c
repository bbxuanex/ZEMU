#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>
#include <stddef.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

// ========== 辅助函数 Helper Functions ==========

// 将整数转换为字符串，存入 buf，返回长度
// Convert integer to string, store in buf, return length
static int itoa_buf(char *buf, int num, int base, int sign)
{
  char *p = buf;
  const char *digits = "0123456789abcdef";
  unsigned int u_num;

  // 处理负数 / Handle negative numbers
  if (sign && base == 10 && num < 0)
  {
    *p++ = '-';
    u_num = (unsigned int)(-num);
  }
  else
  {
    u_num = (unsigned int)num;
  }

  // 特判零 / Special case for zero
  if (u_num == 0)
  {
    *p++ = '0';
    *p = '\0';
    return p - buf;
  }

  // 先逆序写入数字部分 / Write digits in reverse order first
  char *num_start = p;
  while (u_num > 0)
  {
    *p++ = digits[u_num % base];
    u_num /= base;
  }
  *p = '\0';

  // 反转数字部分 / Reverse the digit portion
  char *left = num_start;
  char *right = p - 1;
  while (left < right)
  {
    char tmp = *left;
    *left++ = *right;
    *right-- = tmp;
  }

  return p - buf;
}

// 计算字符串长度（不依赖 strlen，避免循环依赖）
// Calculate string length (no dependency on strlen to avoid circular dependency)
static int my_slen(const char *s)
{
  int len = 0;
  while (*s++)
    len++;
  return len;
}

// 向输出缓冲区安全写入一个字符（带边界检查）
// Safely write one char to output buffer (with bounds check)
static inline void safe_putc(char **out, char *end, char c)
{
  // end == NULL 表示无限制（vsprintf 场景）
  // end == NULL means no limit (vsprintf scenario)
  if (end == NULL || *out < end)
  {
    **out = c;
    (*out)++;
  }
}

// 将字符串写入输出缓冲区，处理宽度和对齐
// Write string to output buffer, handling width and alignment
static void pad_and_write(char **out, char *end, const char *str, int width, int left_align)
{
  int len = my_slen(str);
  int padding = (width > len) ? (width - len) : 0;

  // 右对齐：先填充空格 / Right-align: pad spaces first
  if (!left_align)
  {
    while (padding-- > 0)
      safe_putc(out, end, ' ');
  }

  // 写入内容 / Write content
  while (*str)
  {
    safe_putc(out, end, *str++);
  }

  // 左对齐：后填充空格 / Left-align: pad spaces after
  if (left_align)
  {
    while (padding-- > 0)
      safe_putc(out, end, ' ');
  }
}

// ========== 核心格式化引擎 Core Formatting Engine ==========
// 统一的格式化核心，vsprintf 和 vsnprintf 都调用它
// Unified formatting core, called by both vsprintf and vsnprintf
// end == NULL 表示不限制长度（vsprintf 模式）
// end == NULL means unlimited length (vsprintf mode)
static int format_core(char *out, char *end, const char *fmt, va_list ap)
{
  char *start = out;
  char tmp_buf[128]; // 临时缓冲区，用于数字/浮点转换
                     // Temp buffer for number/float conversion

  while (*fmt)
  {
    // 普通字符，直接输出 / Normal character, output directly
    if (*fmt != '%')
    {
      safe_putc(&out, end, *fmt++);
      continue;
    }

    fmt++; // 跳过 '%' / Skip '%'

    // --- 第一步：解析标志位 Flags (目前只处理 '-') ---
    // --- Step 1: Parse flags (currently only '-') ---
    int left_align = 0;
    if (*fmt == '-')
    {
      left_align = 1;
      fmt++;
    }

    // --- 第二步：解析宽度 Width (如 "20") ---
    // --- Step 2: Parse width (e.g. "20") ---
    int width = 0;
    while (*fmt >= '0' && *fmt <= '9')
    {
      width = width * 10 + (*fmt - '0');
      fmt++;
    }

    // --- 第三步：根据类型转换并输出 ---
    // --- Step 3: Convert by type and output ---
    switch (*fmt)
    {
    case 'd':
    {
      int val = va_arg(ap, int);
      itoa_buf(tmp_buf, val, 10, 1); // signed=1
      pad_and_write(&out, end, tmp_buf, width, left_align);
      break;
    }
    case 'u':
    {
      int val = va_arg(ap, int);
      itoa_buf(tmp_buf, val, 10, 0); // signed=0, 无符号十进制 / unsigned decimal
      pad_and_write(&out, end, tmp_buf, width, left_align);
      break;
    }
    case 'x':
    {
      int val = va_arg(ap, int);
      itoa_buf(tmp_buf, val, 16, 0); // 十六进制 / hexadecimal
      pad_and_write(&out, end, tmp_buf, width, left_align);
      break;
    }
    case 's':
    {
      char *str = va_arg(ap, char *);
      if (!str)
        str = "(null)"; // 防御空指针 / Guard against NULL pointer
      pad_and_write(&out, end, str, width, left_align);
      break;
    }
    case 'c':
    {
      int v = va_arg(ap, int); // 注意：默认整型提升，必须取 int
      tmp_buf[0] = (char)v;
      tmp_buf[1] = '\0';
      pad_and_write(&out, end, tmp_buf, width, left_align);
      break;
    }

    case '%':
    {
      // 处理 "%%"，输出一个 '%'
      // Handle "%%", output a literal '%'
      safe_putc(&out, end, '%');
      break;
    }
    default:
    {
      // 未知格式符，原样输出 / Unknown specifier, output as-is
      safe_putc(&out, end, '%');
      safe_putc(&out, end, *fmt);
      break;
    }
    }
    fmt++;
  }

  *out = '\0';
  return out - start;
}

// ========== 对外接口 Public API ==========

int vsprintf(char *out, const char *fmt, va_list ap)
{
  // end 传 NULL，表示不做边界检查（无限缓冲区）
  // Pass NULL as end, meaning no bounds check (unlimited buffer)
  return format_core(out, NULL, fmt, ap);
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
  if (n == 0)
    return 0;
  // end 指向最后一个可写位置（预留 \0）
  // end points to the last writable position (reserve space for \0)
  return format_core(out, out + n - 1, fmt, ap);
}

int printf(const char *fmt, ...)
{
  char buf[4096];
  va_list ap;
  va_start(ap, fmt);
  int len = vsprintf(buf, fmt, ap);
  va_end(ap);
  for (int i = 0; i < len; ++i)
  {
    putch(buf[i]);
  }
  return len;
}

int sprintf(char *out, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int len = vsprintf(out, fmt, ap);
  va_end(ap);
  return len;
}

int snprintf(char *out, size_t n, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return len;
}

#endif
