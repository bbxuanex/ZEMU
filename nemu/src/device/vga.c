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

#include <common.h>
#include <device/map.h>

#define SCREEN_W (MUXDEF(CONFIG_VGA_SIZE_800x600, 800, 400))
#define SCREEN_H (MUXDEF(CONFIG_VGA_SIZE_800x600, 600, 300))

static uint32_t screen_width()
{
  return MUXDEF(CONFIG_TARGET_AM, io_read(AM_GPU_CONFIG).width, SCREEN_W);
}

static uint32_t screen_height()
{
  return MUXDEF(CONFIG_TARGET_AM, io_read(AM_GPU_CONFIG).height, SCREEN_H);
}

static uint32_t screen_size()
{
  return screen_width() * screen_height() * sizeof(uint32_t);
}

static void *vmem = NULL;
static uint32_t *vgactl_port_base = NULL;

#ifdef CONFIG_VGA_SHOW_SCREEN
#ifndef CONFIG_TARGET_AM
#include <SDL2/SDL.h>

static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;

static void init_screen()
{
  SDL_Window *window = NULL;
  char title[128];
  sprintf(title, "%s-NEMU", str(__GUEST_ISA__));
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(
      SCREEN_W * (MUXDEF(CONFIG_VGA_SIZE_400x300, 2, 1)),
      SCREEN_H * (MUXDEF(CONFIG_VGA_SIZE_400x300, 2, 1)),
      0, &window, &renderer);
  SDL_SetWindowTitle(window, title);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STATIC, SCREEN_W, SCREEN_H);
  SDL_RenderPresent(renderer);
}

static inline void update_screen()
{
  SDL_UpdateTexture(texture, NULL, vmem, SCREEN_W * sizeof(uint32_t));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}
#else
static void init_screen() {}

static inline void update_screen()
{
  io_write(AM_GPU_FBDRAW, 0, 0, vmem, screen_width(), screen_height(), true);
}
#endif
#endif

void vga_update_screen()
{
  // TODO: call `update_screen()` when the sync register is non-zero,
  // then zero out the sync register
  // 1. 读取同步寄存器的值
  // vgactl_port_base 是显卡控制寄存器的基地址数组 (uint32_t*)
  // [0] 是屏幕大小 (width/height)
  // [1] 是同步信号 (sync) -> 对应 SYNC_ADDR
  uint32_t sync = vgactl_port_base[1];

  // 2. 如果同步信号不为0，说明软件(AM)要求刷新屏幕
  if (sync != 0)
  {
    update_screen();         // 调用内部函数，将显存内容画到 SDL 窗口上
    vgactl_port_base[1] = 0; // 3. 清零同步信号，表示“任务已完成”
  }
}

static void vga_ctl_io_handler(uint32_t offset, int len, bool is_write)
{
  assert(offset == 0 || offset == 4);
  if (is_write && offset == 4)
  {                      // 偏移量 4 对应 SYNC 寄存器
    vga_update_screen(); // 核心：收到同步信号，刷新屏幕！
  }
}

void init_vga()
{
  vgactl_port_base = (uint32_t *)new_space(8);
  vgactl_port_base[0] = (screen_width() << 16) | screen_height();
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map("vgactl", CONFIG_VGA_CTL_PORT, vgactl_port_base, 8, vga_ctl_io_handler);
#else
  add_mmio_map("vgactl", CONFIG_VGA_CTL_MMIO, vgactl_port_base, 8, vga_ctl_io_handler);
#endif

  vmem = new_space(screen_size());
  add_mmio_map("vmem", CONFIG_FB_ADDR, vmem, screen_size(), NULL);
  IFDEF(CONFIG_VGA_SHOW_SCREEN, init_screen());
  IFDEF(CONFIG_VGA_SHOW_SCREEN, memset(vmem, 0, screen_size()));
}
