// 调试辅助程序
#include "_public.h"

void EXIT(int sig)
{
    printf("sig = %d \n", sig);

    if(sig == 2) exit(0);
}

CPActive Active;

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        printf("Using:./book procname timout\n");
        return 0;
    }

    // 设置 2 和 15 的信号
    signal(2, EXIT);

    signal(15, EXIT);


    Active.AddPInfo(atoi(argv[2]), argv[1]);

    while(true)
    {
        // Active.UptATime();

        sleep(10);
    }

    return 0;
}