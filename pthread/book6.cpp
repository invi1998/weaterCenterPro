// 线程参数传递
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

void * thmain1(void * arg);

void * thmain2(void * arg);

int var = 0;

struct st_ret
{
    int retcode;    // 返回代码
    char message[1024];     // 返回内容
};


int main(int argc, char* argv[])
{

    pthread_t thid1=0, thid2=0;

    // 创建线程
    if(pthread_create(&thid1, NULL, thmain1, NULL) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }

    if(pthread_create(&thid2, NULL, thmain2, NULL) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }

    sleep(10);

    // 等待子线程退出
    printf("join...\n");

    void *pv = nullptr;
    pthread_join(thid1, &pv);
    printf("线程1ret = %ld\n", (long)pv);

    pthread_join(thid2, &pv);
    printf("线程2ret = %ld\n", (long)pv);


    printf("join-ok\n");
}

void * thmain1(void * arg)
{
    for(int i = 0; i < 3; i++)
    {
        printf("线程1-【%d】\n", i);
    }
    return (void*)10;
}

void * thmain2(void * arg)
{
    for(int i = 0; i < 5; i++)
    {
        printf("线程2-【%d】\n", i);
    }
    return (void*)11;
}
