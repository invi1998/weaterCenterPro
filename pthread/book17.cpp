// 线程参数传递
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

// 线程入口函数
void * thmain(void * arg);

int var = 0;

sem_t sem;      // 声明信号量

int main(int argc, char* argv[])
{

    sem_init(&sem, 0, 1);

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

    printf("var = %d\n", var);

    sem_destroy(&sem);
}

void * thmain(void * arg)
{
   for(int i = 0; i < 100000; i++)
   {
       sem_wait(&sem);          // 信号量P操作（信号量值-1）
       var++;
       sem_post(&sem);          // 信号量V操作（信号量值+1）
   }
    return (void*)10;
}

