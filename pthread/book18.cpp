#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;      // 声明互斥锁并初始化
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;         // 声明条件变量并初始化

void * thmain1(void * arg);         // 线程1入口函数
void * thmain2(void * arg);         // 线程2入口函数

int main(int argc, char* argv[])
{
    pthread_t thid1=0,thid2=0;

    // 创建线程
    if(pthread_create(&thid1, NULL, thmain1, NULL) != 0)
    {
        printf("线程创建失败\n");
    }

    sleep(1);

    if(pthread_create(&thid2, NULL, thmain2, NULL) != 0)
    {
        printf("线程创建失败\n");
    }

    // 等待子线程退出
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);


    pthread_mutex_destroy(&mutex);      // 销毁互斥锁
    pthread_cond_destroy(&cond);        // 销毁条件变量

}

void *thmain1(void * arg)
{
    printf("线程1申请互斥锁...\n");
    pthread_mutex_lock(&mutex);
    printf("线程1申请互斥锁成功\n");

    printf("线程1开始等待条件变量信号...\n");
    pthread_cond_wait(&cond, &mutex);               // 等待条件变量信号
    printf("线程1等待条件变量信号成功\n");

    return NULL;
}

void *thmain2(void * arg)
{
    pthread_cond_signal(&cond);

    sleep(1);

    printf("线程2申请互斥锁...\n");
    pthread_mutex_lock(&mutex);
    printf("线程2申请互斥锁成功\n");

    printf("线程2开始等待条件变量信号...\n");
    pthread_cond_wait(&cond, &mutex);               // 等待条件变量信号
    printf("线程2等待条件变量信号成功\n");

    return NULL;
}
