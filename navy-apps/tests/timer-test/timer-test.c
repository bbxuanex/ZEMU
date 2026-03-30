#include <sys/time.h>
#include <stdio.h>
int main(void)
{
    struct timeval tv;
    unsigned long long last = 0;

    while (1)
    {
        _gettimeofday(&tv, NULL);

        unsigned long long now = (unsigned long long)tv.tv_usec + (unsigned long long)tv.tv_sec * (unsigned long long)1000000;

        if (last == 0)
        {
            last = now;
            continue;
        }
        if (now - last >= 500000)
        {
            printf("time has just passed 0.5s!\n");
            last = now;
        }
    }
    return 0;
}