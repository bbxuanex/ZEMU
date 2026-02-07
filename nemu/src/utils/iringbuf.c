//----  write in     nemu/include/utils.h  ----

#include <common.h>
#define IRINGBUF_SIZE 16

typedef struct
{
    word_t pc;
    uint32_t inst;
    char log[128];
} ItraceNode;

static ItraceNode iringbuf[IRINGBUF_SIZE];
static int cur_p = 0;
static bool full = false;

void iringbuf_write(word_t pc, uint32_t inst, char *log)
{
    iringbuf[cur_p].pc = pc;
    iringbuf[cur_p].inst = inst;
    strncpy(iringbuf[cur_p].log, log, 128);
    cur_p = (cur_p + 1) % IRINGBUF_SIZE;
    full = (cur_p == 0) ? true : false;
}
void iringbuf_display()
{
    if (!full && cur_p == 0)
        return;
    printf("=== Ring Buffer Dump ===");
    int i = full ? cur_p : 0;
    int count = full ? IRINGBUF_SIZE : cur_p;
    for (int k = 0; k < count; ++k)
    {
        ItraceNode *p = &iringbuf[i];
        // Actually to avoid the situation where cur_p = 0.
        bool islast = (i == (cur_p - 1 + IRINGBUF_SIZE) % IRINGBUF_SIZE);
        printf("%s 0x%08x:  %s\n", islast ? "--->" : "    ", p->pc, p->log);

        i = (i + 1) % IRINGBUF_SIZE;
    }
}