
#include <common.h>
#include <utils.h>

#define MTRACE_START 0x80000000
#define MTRACE_END 0x80001000

void display_mtrace(char type, paddr_t addr, int len, word_t data)
{
#ifdef CONFIG_MTRACE
    if (addr >= MTRACE_START && addr <= MTRACE_END)
    {
        log_write("[MTRACE] %s addr=" FMT_PADDR " len=%d data=" FMT_WORD "\n",
                  (type == 'r' ? "READ " : "WRITE"),
                  addr, len, data);
    }
#endif
}