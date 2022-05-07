// 线程参数传递
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

void * thmain1(void * arg);

int main(int argc, char* argv[])
{

    pthread_t thid1=0;

    pthread_attr_t attr;        // 声明线程属性的数据结构
    pthread_attr_init(&attr);   // 初始化
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);        // 设置线程的分离属性

    // 创建线程（将线程属性作为参数传递给创建线程函数）
    if(pthread_create(&thid1, &attr, thmain1, NULL) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }

    sleep(10);
    // 等待子线程退出
    printf("join...\n");

    void *pv = nullptr;
    int result = 0;
    result = pthread_join(thid1, &pv);
    printf("线程1ret = %ld, join结果 = %d\n", (long)pv, result);

    printf("join-ok\n");
}

void * thmain1(void * arg)
{
    for(int i = 0; i < 3; i++)
    {
        sleep(1);
        printf("线程1-【%d】\n", i);
    }
    return (void*)10;
}
