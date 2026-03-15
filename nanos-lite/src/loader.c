#include <proc.h>
#include <elf.h>
#include <string.h>

#ifdef __LP64__
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Phdr Elf64_Phdr
#else
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Phdr Elf32_Phdr
#endif

#if defined(__ISA_AM_NATIVE__)
#define EXPECT_TYPE EM_X86_64
#elif defined(__ISA_X86__)
#define EXPECT_TYPE EM_386
#elif defined(__ISA_RISCV32__) || defined(__ISA_RISCV64__)
#define EXPECT_TYPE EM_RISCV
#else
#error Unsupported ISA
#endif

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t get_ramdisk_size();

static uintptr_t loader(PCB *pcb, const char *filename)
{
  Elf_Ehdr ehdr;
  ramdisk_read(&ehdr, 0, sizeof(Elf_Ehdr));

  // check magic number
  assert(*(uint32_t *)ehdr.e_ident == 0x464c457f);
  assert(ehdr.e_machine == EXPECT_TYPE);

  Elf_Phdr phdr;
  for (int i = 0; i < ehdr.e_phnum; ++i)
  {
    // figure out program header's offset and read
    size_t phdr_offset = ehdr.e_phoff + i * sizeof(Elf_Phdr);
    ramdisk_read(&phdr, phdr_offset, sizeof(Elf_Phdr));

    // filter and load segment typed PT_LOAD
    if (phdr.p_type == PT_LOAD)
    {
      // act1
      ramdisk_read((void *)phdr.p_vaddr, phdr.p_offset, phdr.p_filesz);
      // act2
      memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);
    }
  }

  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename)
{
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void (*)())entry)();
}
