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
static int dscb_of_fd;

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

  int ret = read(dscb_of_fd, buf, len);
  return ret ? 1 : 0;
}

void NDL_OpenCanvas(int *w, int *h)
{
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

  dscb_of_fd = open("/dev/events", 0, 0);

  return 0;
}

void NDL_Quit()
{
  time_libst = 0;
}
