#include <fs.h>
#include <unistd.h>
#define ARRAY_LEN (sizeof(file_table) / sizeof(file_table[0]))
typedef size_t (*ReadFn)(void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn)(const void *buf, size_t offset, size_t len);

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

typedef struct
{
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
  size_t open_offset;

} Finfo;

enum
{
  FD_STDIN,
  FD_STDOUT,
  FD_STDERR,
  FD_FB
};

size_t invalid_read(void *buf, size_t offset, size_t len)
{
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len)
{
  panic("should not reach here");
  return 0;
}

size_t serial_write(const void *buf, size_t offset, size_t len);

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
    [FD_STDIN] = {"stdin", 0, 0, invalid_read, invalid_write},
    [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write},
    [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write},
#include "files.h"
};

void init_fs()
{
  // TODO: initialize the size of /dev/fb
}

int fs_open(const char *pathname, int flags, int mode)
{
  assert(pathname != NULL);

  int i = 0;
  while (i < ARRAY_LEN && strcmp(file_table[i].name, pathname))
  {
    ++i;
  }
  assert(i < ARRAY_LEN);

  file_table[i].open_offset = 0;
  return i;
}
size_t fs_write(int fd, const void *buf, size_t count)
{
  if (fd < 0 || fd >= ARRAY_LEN)
    panic("WRONG FD in FS_WRITE");

  size_t num = count;
  size_t ret = -1;
  if (file_table[fd].write == NULL)
  {
    bool is_overflow = false;
    if ((file_table[fd].open_offset) + (count) > (file_table[fd].size))
    {
      is_overflow = true;
    }
    if (is_overflow)
    {
      num = file_table[fd].size - file_table[fd].open_offset;
    }

    ret = (size_t)ramdisk_write(buf, file_table[fd].open_offset + file_table[fd].disk_offset, num);
    file_table[fd].open_offset += num;
  }

  else
    ret = (size_t)(file_table[fd].write)(buf, file_table[fd].disk_offset, count);

  return ret;
}

size_t fs_read(int fd, void *buf, size_t count)
{
  if (fd < 0 || fd >= ARRAY_LEN)
    panic("WRONG FD in FS_READ");
  size_t num = count;
  bool is_overflow = false;

  if ((off_t)(file_table[fd].open_offset) + (off_t)(count) > (off_t)(file_table[fd].size))
  {
    is_overflow = true;
  }
  if (is_overflow)
  {
    num = file_table[fd].size - file_table[fd].open_offset;
  }

  if (fd == 0)
    return 0;

  size_t ret = (size_t)ramdisk_read(buf, file_table[fd].open_offset + file_table[fd].disk_offset, num);
  file_table[fd].open_offset += num;

  return ret;
}

size_t fs_lseek(int fd, size_t offset, int whence)
{
  if (fd < 0 || fd >= ARRAY_LEN)
    panic("WRONG FD in FS_LSEEK");

  size_t new_offset;
  size_t tr_size = file_table[fd].size;
  size_t tr_op = file_table[fd].open_offset;

  switch (whence)
  {
  case SEEK_SET:
    new_offset = offset;
    if (new_offset > tr_size || new_offset < 0)
      return -1;
    else
      file_table[fd].open_offset = new_offset;
    return new_offset;

  case SEEK_CUR:
    new_offset = tr_op + offset;
    if (new_offset > tr_size || new_offset < 0)
      return -1;
    else
      file_table[fd].open_offset = new_offset;
    return new_offset;

  case SEEK_END:
    new_offset = tr_size + offset;
    if (new_offset > tr_size || new_offset < 0)
      return -1;
    else
      file_table[fd].open_offset = new_offset;
    return new_offset;

  default:
    panic("Wrong whence!");
  }
  return 0;
}

int fs_close(int fd)
{
  return 0;
}