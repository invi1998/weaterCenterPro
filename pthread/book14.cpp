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

pthread_spinlock_t spin;           // 声明自旋锁

int main(int argc, char* argv[])
{
    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);        // 初始化锁

    pthread_t thid1=0, thid2 = 0;

    // 创建线程（将线程属性作为参数传递给创建线程函数）
    if(pthread_create(&thid1, NULL, thmain, NULL) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }

    if(pthread_create(&thid2, NULL, thmain, NULL) != 0)
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

    pthread_spin_destroy(&spin);
}

void * thmain(void * arg)
{
    for(int i = 0; i< 100000; i++)
    {
        pthread_spin_lock(&spin);
        var++;
        pthread_spin_unlock(&spin);
    }

    return (void*)10;
}

