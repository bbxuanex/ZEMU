#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

void print_num(char **out, int num, int base, int sign)
{
  char buf[32];
  int index = 0;
  const char *digits = "0123456789abcdef";
  unsigned int u_num;
  if (sign && base == 10 && num < 0)
  {
    **out = '-';
    u_num = (unsigned int)-num;
    (*out)++;
  }
  else
  {
    u_num = (unsigned int)num;
  }
  if (u_num == 0)
  {
    *(*out)++ = '0';
    return;
  }
  while (u_num > 0)
  {
    buf[index++] = digits[u_num % base];
    u_num /= base;
  }
  index--;
  while (index >= 0)
  {
    *(*out)++ = buf[index--];
  }
}

int printf(const char *fmt, ...)
{
  char buf[2048];
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

int vsprintf(char *out, const char *fmt, va_list ap)
{
  char *start = out;
  int val = 0;
  while (*fmt)
  {
    if (*fmt == '%')
    {
      fmt++;

      switch (*fmt)
      {
      case 'd':
        val = va_arg(ap, int);
        print_num(&out, val, 10, 1);
        break;
      case 's':
      {
        char *str = va_arg(ap, char *);
        while (*str)
        {
          *out++ = *str++;
        }
        break;
      }
      case 'u':
        val = va_arg(ap, int);
        print_num(&out, val, 10, 0);
        break;
      case 'x':
        val = va_arg(ap, int);
        print_num(&out, val, 16, 0);
        break;
      }
      fmt++;
    }
    else
    {
      *out++ = *fmt++;
    }
  }
  *out = '\0';
  return out - start;
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

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
  char *start = out;
  char *end = out + n - 1;
  int val = 0;
  while (*fmt)
  {
    if (out >= end)
      break;
    if (*fmt == '%')
    {
      fmt++;
      switch (*fmt)
      {
      case 'd':
        val = va_arg(ap, int);
        print_num(&out, val, 10, 1);
        break;
      case 's':
      {
        char *str = va_arg(ap, char *);
        while (*str && out < end)
        {
          *out++ = *str++;
        }
        break;
      }
      case 'u':
        val = va_arg(ap, int);
        print_num(&out, val, 10, 0);
        break;
      case 'x':
        val = va_arg(ap, int);
        print_num(&out, val, 16, 0);
        break;
      }
      fmt++;
    }
    else
    {
      if (out < end)
      {
        *out++ = *fmt;
      }
      fmt++;
    }
  }
  *out = '\0';
  return out - start;
}

#endif
