#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    void *p0 = sbrk(0);
    void *p1 = sbrk(16);
    void *p2 = sbrk(0);
    printf("p0=%p p1=%p p2=%p\n", p0, p1, p2);

    char *p = malloc(1024);
    if (p)
    {
        p[0] = 'A';
        p[1023] = 'Z';
        printf("malloc ok: %p %c %c\n", p, p[0], p[1023]);
    }
    else
    {
        printf("malloc failed\n");
    }
    return 0;
}
