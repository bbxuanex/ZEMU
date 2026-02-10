#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s)
{
  size_t num = 0;
  while (*s++)
  {
    num++;
  }

  return num;
}

char *strcpy(char *dst, const char *src)
{
  char *start = dst;
  while ((*dst++ = *src++))
  {
  }
  return start;
}

char *strncpy(char *dst, const char *src, size_t n)
{
  size_t i;

  for (i = 0; i < n && src[i] != '\0'; i++)
    dst[i] = src[i];

  for (; i < n; i++)
    dst[i] = '\0';
  return dst;
}

char *strcat(char *dst, const char *src)
{
  char *d = dst;
  while (*d)
    d++;
  while ((*d++ = *src++))
    ;

  return dst;
}

int strcmp(const char *s1, const char *s2)
{
  const unsigned char *S1 = (const unsigned char *)s1;
  const unsigned char *S2 = (const unsigned char *)s2;
  for (; *S1 == *S2 && *S1 && *S2; S1++, S2++)
    ;
  return (int)*S1 - (int)*S2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
  if (n == 0)
    return 0;
  unsigned char *S1 = (unsigned char *)s1;
  unsigned char *S2 = (unsigned char *)s2;

  while (n-- > 0)
  {
    if (*S1 != *S2)
      return *S1 - *S2;
    if (*S1 == '\0')
      return 0;
    S1++;
    S2++;
  }
  return 0;
}

void *memset(void *s, int c, size_t n)
{
  unsigned char *p = s;
  while (n--)
  {
    *p++ = (unsigned char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n)
{
  const char *s = src;
  char *d = dst;

  if (s < d && d < s + n)
  {
    s += n;
    d += n;
    while (n-- > 0)
    {
      *--d = *--s;
    }
  }
  else
  {
    while (n-- > 0)
    {
      *d++ = *s++;
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n)
{
  char *d = out;
  const char *s = in;
  while (n--)
  {
    *d++ = *s++;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
  const unsigned char *p1 = s1;
  const unsigned char *p2 = s2;

  while (n--)
  {
    if (*p1 != *p2)
    {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  return 0;
}

#endif
