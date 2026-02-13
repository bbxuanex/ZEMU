#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void)
{
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next / 65536) % 32768;
}

void srand(unsigned int seed)
{
  next = seed;
}

int abs(int x)
{
  return (x < 0 ? -x : x);
}

int atoi(const char *nptr)
{
  int x = 0;
  while (*nptr == ' ')
  {
    nptr++;
  }
  while (*nptr >= '0' && *nptr <= '9')
  {
    x = x * 10 + *nptr - '0';
    nptr++;
  }
  return x;
}

typedef struct block_header
{
  size_t size;
  struct block_header *next;
} header_t;

static header_t *free_list = NULL; // 空闲链表头指针
static void *addr_brk = NULL;      // 模拟 brk 指针，指向未分配区域的边界

static size_t align_up(size_t size)
{
  return (size + 7) & (~7);
}

void *malloc(size_t size)
{
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
  // 0. 边界情况处理
  if (0 == size)
  {
    return NULL;
  }
  // 1. 初始化堆指针 (仅第一次调用时执行)
  if (addr_brk == NULL)
  {
    addr_brk = heap.start;
  }
  // 2. 计算实际需要申请的总大小 (Header + 用户数据 + 对齐)
  size_t total_size = align_up(size + sizeof(header_t));
  // 3. 策略一：先查空闲链表 (First Fit)
  header_t *prev = NULL;
  header_t *curr = NULL;

  while (curr)
  {
    if (curr->size >= total_size)
    { // 找到了足够大的空闲块！
      // 从链表中移除该节点
      if (prev)
      {
        prev->next = curr->next;
      }
      else
      {
        free_list = curr->next;
      }
      // 【进阶思考】：这里如果 curr->size 比 total_size 大很多，
      // 应该进行 Split (切割)，把剩下的部分再插回链表。
      // 为了代码简洁，这里暂时“浪费”掉多余的空间。

      // 返回用户区域指针 (跳过 Header)
      return (void *)(curr + 1);
    }
    prev = curr;
    curr = curr->next;
  }
  // 4. 策略二：链表里没有合适的，向系统堆申请 (Bump Pointer)
  // 检查是否会堆溢出
  if ((uintptr_t)addr_brk + total_size > (uintptr_t)heap.end)
  {
    return NULL;
  }
  // 分配新块
  header_t *new_block = (header_t *)addr_brk;
  new_block->size = total_size;
  // 更新 brk 指针
  addr_brk = (void *)((uintptr_t)addr_brk + total_size);
  // 返回用户区域
  return (void *)(new_block + 1);
#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
  panic("Not implemented");
#endif
  return NULL;
}

void free(void *ptr)
{
  if (ptr == NULL)
    return;
  // 1. 根据用户指针找到 Header
  // 指针运算：ptr 向前回退 sizeof(header_t) 长度
  header_t *block = (header_t *)ptr - 1;

  // 【防御性编程建议】：可以在 Header 里加个 Magic Number 检查 block 是否合法

  // 2. 将该块插入空闲链表
  // 采用“头插法”，最简单，O(1) 复杂度
  block->next = free_list;
  free_list = block;
  // 【进阶思考】：此时内存虽然回收了，但物理上可能是不连续的碎片。
  // 真正的实现需要遍历链表，检查 block 和 block->next 是否在物理上相邻，
  // 如果相邻，就合并它们 (Coalescing)
}

#endif
