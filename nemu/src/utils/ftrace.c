#include <common.h>
#include <elf.h>

#ifdef CONFIG_FTRACE

// 函数符号表
static FuncSymbol func_table[MAX_FUNC_NUM];
static int func_count = 0;

// 调用深度（用于缩进）
static int call_depth = 0;

// 根据地址查找函数名
static const char *find_func_name(paddr_t addr)
{
    for (int i = 0; i < func_count; i++)
    {
        if (addr >= func_table[i].addr &&
            addr < func_table[i].addr + func_table[i].size)
        {
            return func_table[i].name;
        }
    }
    return "???";
}

// 初始化 ftrace，解析 ELF 文件的符号表
void init_ftrace(const char *elf_file)
{
    if (elf_file == NULL)
    {
        Log("No ELF file specified for ftrace.");
        return;
    }

    FILE *fp = fopen(elf_file, "rb");
    Assert(fp != NULL, "Cannot open ELF file '%s'", elf_file);

    // 读取 ELF header
    Elf32_Ehdr ehdr;
    int ret = fread(&ehdr, sizeof(Elf32_Ehdr), 1, fp);
    Assert(ret == 1, "Failed to read ELF header");

    // 验证 ELF magic number
    Assert(ehdr.e_ident[EI_MAG0] == ELFMAG0 &&
               ehdr.e_ident[EI_MAG1] == ELFMAG1 &&
               ehdr.e_ident[EI_MAG2] == ELFMAG2 &&
               ehdr.e_ident[EI_MAG3] == ELFMAG3,
           "Invalid ELF file");

    // 读取 Section Header Table
    Elf32_Shdr *shdr = malloc(ehdr.e_shnum * sizeof(Elf32_Shdr));
    fseek(fp, ehdr.e_shoff, SEEK_SET);
    ret = fread(shdr, sizeof(Elf32_Shdr), ehdr.e_shnum, fp);
    Assert(ret == ehdr.e_shnum, "Failed to read section headers");

    // 找到符号表 (.symtab) 和字符串表 (.strtab)
    Elf32_Shdr *symtab_shdr = NULL;
    Elf32_Shdr *strtab_shdr = NULL;

    for (int i = 0; i < ehdr.e_shnum; i++)
    {
        if (shdr[i].sh_type == SHT_SYMTAB)
        {
            symtab_shdr = &shdr[i];
            // 符号表的 sh_link 指向对应的字符串表
            strtab_shdr = &shdr[symtab_shdr->sh_link];
            break;
        }
    }

    if (symtab_shdr == NULL)
    {
        Log("No symbol table found in ELF file");
        free(shdr);
        fclose(fp);
        return;
    }

    // 读取字符串表
    char *strtab = malloc(strtab_shdr->sh_size);
    fseek(fp, strtab_shdr->sh_offset, SEEK_SET);
    ret = fread(strtab, strtab_shdr->sh_size, 1, fp);
    Assert(ret == 1, "Failed to read string table");

    // 读取符号表
    int sym_count = symtab_shdr->sh_size / sizeof(Elf32_Sym);
    Elf32_Sym *symtab = malloc(symtab_shdr->sh_size);
    fseek(fp, symtab_shdr->sh_offset, SEEK_SET);
    ret = fread(symtab, sizeof(Elf32_Sym), sym_count, fp);
    Assert(ret == sym_count, "Failed to read symbol table");

    // 提取所有函数符号
    func_count = 0;
    for (int i = 0; i < sym_count; i++)
    {
        // 只关心类型为 FUNC 的符号
        if (ELF32_ST_TYPE(symtab[i].st_info) == STT_FUNC)
        {
            if (func_count >= MAX_FUNC_NUM)
            {
                Log("Warning: too many functions, truncating...");
                break;
            }
            strncpy(func_table[func_count].name,
                    strtab + symtab[i].st_name,
                    MAX_FUNC_NAME_LEN - 1);
            func_table[func_count].name[MAX_FUNC_NAME_LEN - 1] = '\0';
            func_table[func_count].addr = symtab[i].st_value;
            func_table[func_count].size = symtab[i].st_size;
            func_count++;
        }
    }

    Log("Loaded %d function symbols for ftrace", func_count);

    free(symtab);
    free(strtab);
    free(shdr);
    fclose(fp);
}

// 记录函数调用
void ftrace_call(paddr_t pc, paddr_t target)
{
    // 打印缩进
    printf("0x%08x: ", pc);
    for (int i = 0; i < call_depth; i++)
    {
        printf("  ");
    }
    printf("call [%s@0x%08x]\n", find_func_name(target), target);
    call_depth++;
}

// 记录函数返回
void ftrace_ret(paddr_t pc)
{
    call_depth--;
    if (call_depth < 0)
        call_depth = 0; // 防止下溢

    printf("0x%08x: ", pc);
    for (int i = 0; i < call_depth; i++)
    {
        printf("  ");
    }
    printf("ret  [%s]\n", find_func_name(pc));
}

#endif
