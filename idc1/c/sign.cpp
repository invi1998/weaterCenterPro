#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

// 信号处理函数
void EXIT(int sig)
{
    printf("接收到信号：%d, 程序即将退出\n", sig);

    // 程序退出善后代码
    exit(0);
}

// 时钟信号处理函数
void alarmfunc(int num)
{
    printf("接收到了%d时钟信号 \n", num);
    alarm(3);       // 在处理函数中新增这样一行代码，就可以实现没3s闹钟触发一次，而不是之前的只闹一次
}

int main()
{
    for(int i = 1; i < 64; i++)
    {
        signal(i, SIG_IGN);     // 捕获全部的64个信号量，并将这些进行忽略
    }

    // 设置信号 2 和 信号15 的处理函数,该处理函数命名为EXIT
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    while(1)
    {
        printf("执行了一次任务 \n");
        sleep(1);
    }
}