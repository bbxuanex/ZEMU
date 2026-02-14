#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init()
{
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg)
{
  uint32_t screen_size = inl(VGACTL_ADDR);

  uint32_t w = screen_size >> 16;
  uint32_t h = screen_size & 0xffff;

  *cfg = (AM_GPU_CONFIG_T){
      .present = true, .has_accel = false, .width = w, .height = h, .vmemsz = 0};
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl)
{
  int x = ctl->x, y = ctl->y, w = ctl->w, h = ctl->h;
  if (!ctl->pixels && w == 0 && h == 0 && ctl->sync)
  {
    // 特殊情况：只同步，不绘图
    outl(SYNC_ADDR, 1);
    return;
  }

  uint32_t *pixels = ctl->pixels;
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  uint32_t screen_w = inl(VGACTL_ADDR) >> 16; // 注意这里的高低位要和 config 保持一致

  for (int i = 0; i < h; i++)
  {
    for (int j = 0; j < w; j++)
    {
      fb[(y + i) * screen_w + (x + j)] = pixels[i * w + j];
    }
  }
  if (ctl->sync)
  {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status)
{
  status->ready = true;
}
