#include <am.h>
#include <nemu.h>
#include <klib.h>

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

  // 1. 只有当长宽都大于0且有像素数据时，才需要绘制
  if (w > 0 && h > 0 && ctl->pixels != NULL)
  {
    uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
    uint32_t *pixels = ctl->pixels;
    int screen_w = inl(VGACTL_ADDR) >> 16; // 获取屏幕宽度

    for (int i = 0; i < h; i++)
    {
      // 目标地址：显存基址 + (当前行号 * 屏幕宽) + x偏移
      // 源地址：像素基址 + (当前行号 * 图片宽)
      // 注意：这里必须是 + x，不能是 + i
      memcpy(&fb[(y + i) * screen_w + x], &pixels[i * w], w * sizeof(uint32_t));
    }
  }

  // 2. 绘制完成后，如果要求同步，再写寄存器
  if (ctl->sync)
  {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status)
{
  status->ready = true;
}
