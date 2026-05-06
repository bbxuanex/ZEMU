#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;
static unsigned long long time_libst;
static int events_fd, sysdisp_fd, fb_fd;
static int canvas_w, canvas_h;

static inline uint32_t convert_timeval_to_ms(struct timeval *tv)
{
  return (uint32_t)(unsigned long long)tv->tv_sec * 1000 + (unsigned long long)tv->tv_usec / 1000;
}

uint32_t NDL_GetTicks()
{
  unsigned long long time;
  unsigned long long time_for_media;
  struct timeval unproc_time;

  gettimeofday(&unproc_time, NULL);

  time = convert_timeval_to_ms(&unproc_time);
  time_for_media = time - time_libst;

  return (uint32_t)time_for_media;
}

int NDL_PollEvent(char *buf, int len)
{

  int ret = read(events_fd, buf, len);

  return ret ? 1 : 0;
}

void NDL_OpenCanvas(int *w, int *h)
{
  char buf[128];
  int nread = read(sysdisp_fd, buf, sizeof(buf) - 1);
  if (nread > 0)
  {
    buf[nread] = '\0';
  }

  close(sysdisp_fd);

  sscanf(buf, "WIDTH : %d\nHEIGHT:%d", &screen_w, &screen_h);

  // if width and height were assigned as zero,
  // then reassigned screen_size to them
  if (*w == 0 && *h == 0)
  {
    *w = screen_w;
    *h = screen_h;
  }

  // boundary check,
  if (*w > screen_w)
    *w = screen_w;
  if (*h > screen_h)
    *h = screen_h;

  // memorize the value of width and height,
  // for drawrect later,
  canvas_w = *w;
  canvas_h = *h;

  printf("NDL: Screen size is %d x %d, Canvas size is %d x %d\n",
         screen_w, screen_h, canvas_w, canvas_h);

  if (getenv("NWM_APP"))
  {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w;
    screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1)
    {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0)
        continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0)
        break;
    }
    close(fbctl);
  }
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h)
{
  int start_ro = (screen_h - canvas_h) / 2;
  int start_co = (screen_w - canvas_w) / 2;
  int re_x = x + start_co;
  int re_y = y + start_ro;

  size_t offset = (screen_w * re_y + re_x);

  for (int i = 0; i < h; ++i)
  {
    lseek(fb_fd, (offset + i * screen_w) * 4, SEEK_SET);
    write(fb_fd, pixels + i * w, w * 4);
  }
}

void NDL_OpenAudio(int freq, int channels, int samples)
{
}

void NDL_CloseAudio()
{
}

int NDL_PlayAudio(void *buf, int len)
{
  return 0;
}

int NDL_QueryAudio()
{
  return 0;
}

int NDL_Init(uint32_t flags)
{
  if (getenv("NWM_APP"))
  {
    evtdev = 3;
  }
  static struct timeval libst_time;
  gettimeofday(&libst_time, NULL);

  time_libst = convert_timeval_to_ms(&libst_time);

  events_fd = open("/dev/events", 0, 0);
  sysdisp_fd = open("/proc/dispinfo", 0, 0);
  fb_fd = open("/dev/fb", 0, 0);

  return 0;
}

void NDL_Quit()
{
  time_libst = 0;
}
