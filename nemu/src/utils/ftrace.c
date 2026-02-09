#include <common.h>
#include <elf.h>

#ifdef CONFIG_FTRACE

static FuncSymbol func_table[MAX_FUNC_NUM];
static int func_count = 0;

// depth
static int call_depth = 0;

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

// Initialize ftrace， parse ELF symbol table
void init_ftrace(const char *elf_file)
{
    if (elf_file == NULL)
    {
        Log("No ELF file specified for ftrace.");
        return;
    }

    FILE *fp = fopen(elf_file, "rb");
    Assert(fp != NULL, "Cannot open ELF file '%s'", elf_file); // protective coding ...

    // read ELF header
    Elf32_Ehdr ehdr;
    int ret = fread(&ehdr, sizeof(Elf32_Ehdr), 1, fp);
    Assert(ret == 1, "Failed to read ELF header");

    // check ELF magic number
    Assert(ehdr.e_ident[EI_MAG0] == ELFMAG0 &&
               ehdr.e_ident[EI_MAG1] == ELFMAG1 &&
               ehdr.e_ident[EI_MAG2] == ELFMAG2 &&
               ehdr.e_ident[EI_MAG3] == ELFMAG3,
           "Invalid ELF file");

    // read Section Header Table
    Elf32_Shdr *shdr = malloc(ehdr.e_shnum * sizeof(Elf32_Shdr));
    fseek(fp, ehdr.e_shoff, SEEK_SET);
    ret = fread(shdr, sizeof(Elf32_Shdr), ehdr.e_shnum, fp);
    Assert(ret == ehdr.e_shnum, "Failed to read section headers");

    // find symbol table (.symtab) and string table (.strtab)
    Elf32_Shdr *symtab_shdr = NULL;
    Elf32_Shdr *strtab_shdr = NULL;

    for (int i = 0; i < ehdr.e_shnum; i++)
    {
        if (shdr[i].sh_type == SHT_SYMTAB)
        {
            symtab_shdr = &shdr[i];
            break;
        }
    }

    // pass 2: fallback to DYNSYM
    if (symtab_shdr == NULL)
    {
        for (int i = 0; i < ehdr.e_shnum; i++)
        {
            if (shdr[i].sh_type == SHT_DYNSYM)
            {
                symtab_shdr = &shdr[i];
                break;
            }
        }
    }

    if (symtab_shdr == NULL)
    {
        Log("No symbol table found in ELF file, ftrace will print raw addresses");
        free(shdr);
        fclose(fp);
        return;
    }
    else
    {
        if (symtab_shdr->sh_link >= ehdr.e_shnum)
        {
            panic("bad sh_link");
        }
        strtab_shdr = &shdr[symtab_shdr->sh_link];
        if (strtab_shdr->sh_type != SHT_STRTAB)
        {
            panic("symtab link is not STRTAB");
        }
    }

    // read string table & transport
    char *strtab = malloc(strtab_shdr->sh_size);
    fseek(fp, strtab_shdr->sh_offset, SEEK_SET);
    ret = fread(strtab, strtab_shdr->sh_size, 1, fp);
    Assert(ret == 1, "Failed to read string table");

    // read symbol table & transport
    int sym_count = symtab_shdr->sh_size / sizeof(Elf32_Sym);
    Elf32_Sym *symtab = malloc(symtab_shdr->sh_size);
    fseek(fp, symtab_shdr->sh_offset, SEEK_SET);
    ret = fread(symtab, sizeof(Elf32_Sym), sym_count, fp);
    Assert(ret == sym_count, "Failed to read symbol table");

    // extract symbol
    func_count = 0;
    for (int i = 0; i < sym_count; i++)
    {
        // care about symbol->type: FUNC only
        if (ELF32_ST_TYPE(symtab[i].st_info) == STT_FUNC)
        {
            if (func_count >= MAX_FUNC_NUM)
            {
                Log("Warning: too many functions, truncating...");
                break;
            }
            strncpy(func_table[func_count].name,
                    strtab + symtab[i].st_name, // sure：这是一个指针仿数组下标写法，doubt: st.name值是不连续的
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

// record func call
void ftrace_call(paddr_t pc, paddr_t target)
{
    // %*s 的作用是打印指定宽度的字符串
    // call_depth * 2 是宽度，"" 是内容（空字符串）
    // 这样就实现了打印 call_depth * 2 个空格的效果
    Log("0x%08x: %*scall [%s@0x%08x]",
        pc, call_depth * 2, "", find_func_name(target), target);

    call_depth++;
}

// record func return
void ftrace_ret(paddr_t pc)
{
    call_depth--;
    if (call_depth < 0)
        call_depth = 0; // 防止下溢

    Log("0x%08x: %*sret  [%s]",
        pc, call_depth * 2, "", find_func_name(pc));
}

#endif
