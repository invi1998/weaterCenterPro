// 线程参数传递
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

// 线程入口函数
void * thmain(void * arg);

int var = 0;

void handle(int sig);

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;       // 声明并初始化读写锁

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
    printf("join-ok\n");
    printf("var = %d\n", var);

    pthread_rwlock_destroy(&rwlock);
}

void * thmain(void * arg)
{
    for(int i = 0; i< 100; i++)
    {
        printf("线程%lu开始申请读锁...\n", pthread_self());
        pthread_rwlock_rdlock(&rwlock);         // 加读锁
        printf("线程%lu申请读锁成功\n", pthread_self());
        sleep(5);
        pthread_rwlock_unlock(&rwlock);         // 解锁
        printf("线程%lu释放锁\n", pthread_self());

        if(i == 3)
        {
            sleep(30);
        }
    }

    return (void*)10;
}

// 信号15的处理函数
void handle(int sig)
{
    printf("开始申请写锁...\n");
    pthread_rwlock_wrlock(&rwlock);         // 加写锁
    printf("写锁申请成功\n");
    sleep(10);
    pthread_rwlock_unlock(&rwlock);         // 解锁
    printf("写锁已经释放\n");
}

