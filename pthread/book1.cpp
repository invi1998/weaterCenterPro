// 线程创建
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// 声明一个全局变量
int var = 0;

void* thmain1(void *arg);        // 线程入口函数1
void* thmain2(void *arg);        // 线程入口函数2

int main(int argc, char* argv[])
{
    // 线程id，typedef unsigned long pthread_t
    pthread_t thid1 = 0;
    pthread_t thid2 = 0;

    if(pthread_create(&thid1, NULL, thmain1, NULL) != 0)
    {
        printf("线程创建失败！\n");
        exit(-1);
    }

    if(pthread_create(&thid2, NULL, thmain2, NULL) != 0)
    {
        printf("线程创建失败！\n");
        exit(-1);
    }

    // sleep(2);
    // pthread_cancel(thid1);
    // sleep(2);
    // pthread_cancel(thid2);

    // 线程创建成功之后，在主线程中等待子线程的退出
    printf("join...\n");
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    printf("join. ok\n");
}

void func1()
{
    return;
}


// 线程入口函数1
void* thmain1(void * arg)
{
    for(int i = 0; i < 5; i++)
    {
        var = i + 1;
        sleep(1);
        printf("(线程1)全局变量var = (%d) \n", var);

        if(i == 1)
        {
            // pthread_exit((void*)1);
            func1();
        }
    }

    return NULL;
}

void func2()
{
    pthread_exit(0);
}

// 线程入口函数2
void* thmain2(void * arg)
{
    for(int i = 0; i < 5; i++)
    {
        sleep(1);
        printf("（线程2）全局变量var = (%d) \n", var);
        
        if(i == 1)
        {
            // return (void*)1;
            func2();
        }
    }

    return NULL;
}