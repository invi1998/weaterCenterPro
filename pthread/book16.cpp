// 线程参数传递
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

// 线程入口函数
void * thmain(void * arg);

void handle(int sig);

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;         // 声明条件变量并初始化
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;      // 声明互斥锁并初始化

int main(int argc, char* argv[])
{
    signal(15, handle);

    pthread_t thid1=0, thid2 = 0, thid3 = 0;

    // 创建线程（将线程属性作为参数传递给创建线程函数）
    if(pthread_create(&thid1, NULL, thmain, NULL) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }
    sleep(1);
    if(pthread_create(&thid2, NULL, thmain, NULL) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }
    sleep(1);
    if(pthread_create(&thid3, NULL, thmain, NULL) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }

    // 等待子线程退出
    printf("join...\n");
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    pthread_join(thid3, NULL);
    printf("join-ok\n");

    pthread_cond_destroy(&cond);
}

void * thmain(void * arg)
{
    while (true)
    {
        printf("线程%lu等待条件变量信号...\n", pthread_self());
        pthread_cond_wait(&cond, &mutex);
        printf("线程%lu成功接收到条件变量信号\n", pthread_self());
    }
    return (void*)10;
}

// 信号15的处理函数
void handle(int sig)
{
    printf("发送条件信号...\n");
    // pthread_cond_signal(&cond);         // 唤醒等待条件变量的第一个线程
    pthread_cond_broadcast(&cond);          // 唤醒等待条件变量的所有线程
}

