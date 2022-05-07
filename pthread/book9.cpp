// 线程参数传递
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// 线程入口函数
void * thmain1(void * arg);

// 线程清理函数
void thcleanup1(void* arg);
void thcleanup2(void* arg);
void thcleanup3(void* arg);

int main(int argc, char* argv[])
{

    pthread_t thid1=0;

    // 创建线程（将线程属性作为参数传递给创建线程函数）
    if(pthread_create(&thid1, NULL, thmain1, NULL) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }

    sleep(10);
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
    // 在线程入口函数中将线程清理函数入栈
    pthread_cleanup_push(thcleanup1, NULL);
    pthread_cleanup_push(thcleanup2, NULL);
    pthread_cleanup_push(thcleanup3, NULL);

    for(int i = 0; i < 3; i++)
    {
        sleep(1);
        printf("线程1-【%d】\n", i);
    }

    pthread_cleanup_pop(1);
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(1);
    return (void*)10;
}

// 线程清理函数
void thcleanup1(void* arg)
{
    // 在这里释放资源，关闭文件，断开连接，回滚事务，释放锁等
    printf("cleanup 1 ok\n");
}

// 线程清理函数
void thcleanup2(void* arg)
{
    // 在这里释放资源，关闭文件，断开连接，回滚事务，释放锁等
    printf("cleanup 2 ok\n");
}

// 线程清理函数
void thcleanup3(void* arg)
{
    // 在这里释放资源，关闭文件，断开连接，回滚事务，释放锁等
    printf("cleanup 3 ok\n");
}
