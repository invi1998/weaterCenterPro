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

    // 创建线程
    if(pthread_create(&thid1, NULL, thmain1, NULL) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }

    // 设置线程分离
    // pthread_detach(thid1);

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
    pthread_detach(pthread_self());
    for(int i = 0; i < 3; i++)
    {
        sleep(1);
        printf("线程1-【%d】\n", i);
    }
    return (void*)10;
}
