// 线程参数传递
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

void * thmain1(void * arg);
void * thmain2(void * arg);
void * thmain3(void * arg);
void * thmain4(void * arg);
void * thmain5(void * arg);

int var = 0;


int main(int argc, char* argv[])
{

    pthread_t thid1=0,thid2=0,thid3=0,thid4=0,thid5=0;

    // 创建线程
    int *var1 = new int; *var1 = 1;
    if(pthread_create(&thid1, NULL, thmain1, var1) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }
    int *var2 = new int; *var2 = 2;
    if(pthread_create(&thid2, NULL, thmain2, var2) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }
    int *var3 = new int; *var3 = 3;
    if(pthread_create(&thid3, NULL, thmain3, var3) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }
    int *var4 = new int; *var4 = 4;
    if(pthread_create(&thid4, NULL, thmain4, var4) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }
    int *var5 = new int; *var5 = 5;
    if(pthread_create(&thid5, NULL, thmain5, var5) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }

    // 等待子线程退出
    printf("join...\n");
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    pthread_join(thid3, NULL);
    pthread_join(thid4, NULL);
    pthread_join(thid5, NULL);
    printf("join-ok\n");
}

void * thmain1(void * arg)
{
    printf("线程1-var=%d\n", *(int*)arg);
    delete (int*)arg;
    return NULL;
}
void * thmain2(void * arg)
{
    printf("线程2-var=%d\n", *(int*)arg);
    delete (int*)arg;
    return NULL;
}
void * thmain3(void * arg)
{
    printf("线程3-var=%d\n", *(int*)arg);
    delete (int*)arg;
    return NULL;
}
void * thmain4(void * arg)
{
    printf("线程4-var=%d\n", *(int*)arg);
    delete (int*)arg;
    return NULL;
}
void * thmain5(void * arg)
{
    printf("线程5-var=%d\n", *(int*)arg);
    delete (int*)arg;
    return NULL;
}