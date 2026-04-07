#include <stdio.h>
#include <NDL.h>
int main(void)
{
    NDL_Init(0);
    uint32_t last = NDL_GetTicks();

    while (1)
    {
        uint32_t now = NDL_GetTicks();

        if (last == 0)
        {
            last = now;
            continue;
        }
        if (now - last >= 500)
        {
            printf("time has just passed 0.5s!\n");
            printf("now time is %u\n", now);
            last = now;
        }
    }
    NDL_Quit();
    return 0;
}