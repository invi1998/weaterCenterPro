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
    int ii = 10;            // 整形变量是4字节
    void* pv = nullptr;     // 指针占用的内存是8字节，用于存放变量的地址
    pv = &ii;               // 正常来说，指针变量存放的是变量的地址
    printf("pv=%p\n", pv); 

    // 下面这个代码用于演示用指针存放整形数字，然后再讲数据给恢复出来，以用于演示线程中整形变量的传递
    pv = (void*)(long)ii;       // 先把4字节整形变量ii转为8字节长整形，然后将其转为void*(地址类型)
    printf("pv=%p\n", pv);

    int jj = 0;
    jj = (int)(long)pv;        // 将pv这个指针保存的内容转为长整型，然后转为整形赋值给jj
    printf("jj = %d\n", jj);

    pthread_t thid1=0,thid2=0,thid3=0,thid4=0,thid5=0;

    // 创建线程
    var = 1;
    if(pthread_create(&thid1, NULL, thmain1, (void*)(long)var) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }
    var = 2;
    if(pthread_create(&thid2, NULL, thmain2, (void*)(long)var) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }
    var = 3;
    if(pthread_create(&thid3, NULL, thmain3, (void*)(long)var) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }
    var = 4;
    if(pthread_create(&thid4, NULL, thmain4, (void*)(long)var) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }
    var = 5;
    if(pthread_create(&thid5, NULL, thmain5, (void*)(long)var) != 0)
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
    printf("线程1-var=%d\n", (int)(long)arg);
    return NULL;
}
void * thmain2(void * arg)
{
    printf("线程2-var=%d\n", (int)(long)arg);
    return NULL;
}
void * thmain3(void * arg)
{
    printf("线程3-var=%d\n", (int)(long)arg);
    return NULL;
}
void * thmain4(void * arg)
{
    printf("线程4-var=%d\n", (int)(long)arg);
    return NULL;
}
void * thmain5(void * arg)
{
    printf("线程5-var=%d\n", (int)(long)arg);
    return NULL;
}