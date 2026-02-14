#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init()
{
  int i;

  // 这里的 w 和 h 不要写死 400/300，而是要动态获取！
  // 方法 A: 直接再读一次寄存器 (简单粗暴)
  uint32_t screen_size = inl(VGACTL_ADDR);
  int w = screen_size & 0xffff;
  int h = screen_size >> 16;

  // ... 讲义里的测试代码 ...
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  for (i = 0; i < w * h; i++)
    fb[i] = i; // 写入颜色数据

  outl(SYNC_ADDR, 1); // 触发同步
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg)
{
  uint32_t screen_size = inl(VGACTL_ADDR);

  uint32_t h = screen_size >> 16;
  uint32_t w = screen_size & 0xffff;

  *cfg = (AM_GPU_CONFIG_T){
      .present = true, .has_accel = false, .width = w, .height = h, .vmemsz = 0};
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl)
{
  if (ctl->sync)
  {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status)
{
  status->ready = true;
}
