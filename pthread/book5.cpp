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

    // 等待子线程退出
    printf("join...\n");

    void *pv = nullptr;

    pthread_join(thid1, &pv);

    printf("ret = %ld\n", (long)pv);

    printf("----------------------\n");

    struct st_ret *ret = new st_ret;
    pthread_join(thid2, (void**)&ret);
    // 把结构体内容显示出来
    printf("返回代码 = %d\n返回内容 = %s\n", ret->retcode, ret->message);
    // 释放内存
    delete ret;

    printf("join-ok\n");
}

void * thmain1(void * arg)
{
    return (void*)10;
}

void * thmain2(void * arg)
{
    // 注意：如果用结构体的地址作为线程返回值，必须保证在线程主函数结束后，地址仍然是有效的，
    // 所以，要采用动态分配内存的方法，不能用局部变量，因为局部变量在线程函数执行退出后就被释放了，
    // 那么你在主线程中再访问这个被释放的内存就是非法的，所以要在线程函数中动态分配内存
    struct st_ret *ret = new st_ret;
    ret->retcode = 1024;
    strcpy(ret->message, "测试内容");

    pthread_exit((void*)ret);
}
