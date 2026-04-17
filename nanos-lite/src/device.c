#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
#define MULTIPROGRAM_YIELD() yield()
#else
#define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
    [AM_KEY_NONE] = "NONE",
    AM_KEYS(NAME)};

static size_t sys_width;
static size_t sys_height;
static size_t sys_size;

size_t serial_write(const void *buf, size_t offset, size_t len)
{
  char *std_arr = (char *)buf;
  if (!std_arr && len > 0)
  {
    return -1;
  }
  for (size_t i = 0; i < len; ++i)
  {
    putch(std_arr[i]);
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len)
{
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  if (ev.keycode == AM_KEY_NONE)
    return 0;

  const char *dscb_of_key = keyname[ev.keycode];
  char *dscb_of_motion = ev.keydown ? "kd" : "ku";

  int ret = snprintf(buf, len, "%s %s\n", dscb_of_motion, dscb_of_key);

  return ret < len ? ret : len;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len)
{
  AM_GPU_CONFIG_T ev = io_read(AM_GPU_CONFIG);
  int sys_width = ev.width;
  int sys_height = ev.height;

  int ret = snprintf(buf, len, "WIDTH : %d\nHEIGHT:%d", sys_width, sys_height);

  return ret < len ? ret : len;
}

size_t fb_write(const void *buf, size_t offset, size_t len)
{
  size_t start_pix = offset / 4;
  size_t num = len;

  int y = start_pix / sys_width;
  int x = start_pix % sys_width;

  if (x >= sys_width || y >= sys_height)
    return 0;
  if (len + offset > sys_size)
    num = sys_size - offset;

  size_t num_pix = num / 4;
  io_write(AM_GPU_FBDRAW, x, y, (void *)buf, (int)num_pix, 1, true);

  return num;
}

void init_device()
{
  Log("Initializing devices...");
  ioe_init();
  sys_width = io_read(AM_GPU_CONFIG).width;
  sys_height = io_read(AM_GPU_CONFIG).height;
  sys_size = io_read(AM_GPU_CONFIG).vmemsz;
}
