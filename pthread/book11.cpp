// 线程参数传递
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

// 线程入口函数
void * thmain1(void * arg);

// 信号处理函数
void func1(int sig)
{
    printf("func1 catch signal %d\n", sig);
}

void func2(int sig)
{
    printf("func2 catch signal %d\n", sig);
}

int main(int argc, char* argv[])
{

    signal(2, func1);

    pthread_t thid1=0;

    // 创建线程（将线程属性作为参数传递给创建线程函数）
    if(pthread_create(&thid1, NULL, thmain1, NULL) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }

    // 等待子线程退出
    printf("join...\n");
    pthread_join(thid1, NULL);
    printf("join-ok\n");
}

void * thmain1(void * arg)
{
    signal(2, func2);

    printf("sleep begin...\n");
    sleep(10);
    printf("sleep ok\n");

    return (void*)10;
}

