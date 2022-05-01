#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <iostream>
#include <vector>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;      // 声明互斥锁并初始化
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;         // 声明条件变量并初始化

struct st_message
{
    int mesgid;         // 消息的ID
    char message[1024]; // 消息的内容
};

std::vector<struct st_message> vcache;      // 用vector容器做缓存

void incache(int sig);          // 生产者（数据入队列）
void *outcache(void* arg);      // 消费者（数据出队列线程主函数）

int main(int argc, char* argv[])
{

    signal(15, incache);        // 接收15信号，调用生产者函数

    // 创建3个消费者线程
    pthread_t thid1,thid2,thid3;
    pthread_create(&thid1, NULL, outcache, NULL);
    pthread_create(&thid2, NULL, outcache, NULL);
    pthread_create(&thid3, NULL, outcache, NULL);

    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    pthread_join(thid3, NULL);

    pthread_mutex_destroy(&mutex);      // 销毁互斥锁
    pthread_cond_destroy(&cond);        // 销毁条件变量

}

void incache(int sig)          // 生产者（数据入队列）
{
    static int mesgid = 1;      // 消息计数器
    struct st_message stmesg;       // 消息内容
    memset(&stmesg, 0, sizeof(struct st_message));

    pthread_mutex_lock(&mutex);         // 给缓存队列加锁

    // 生产数据，放入缓存队列
    stmesg.mesgid = mesgid++;
    vcache.push_back(stmesg);

    pthread_mutex_unlock(&mutex);       // 给缓存队列解锁
    pthread_cond_broadcast(&cond);         // 发送条件信号，激活全部线程
}

void *outcache(void* arg)      // 消费者（数据出队列线程主函数）
{
    struct st_message stmsg;

    while (true)
    {
        pthread_mutex_lock(&mutex);         // 给缓存队列加锁

        // 如果缓存队列为空，等待，用while防止条件变量被虚假唤醒
        while(vcache.size() == 0)
        {
            pthread_cond_wait(&cond, &mutex);
        }

        // 从缓存对垒中获取第一条激励，然后删除该记录
        memcpy(&stmsg, &vcache[0], sizeof(struct st_message));     // 内存拷贝

        vcache.erase(vcache.begin());

        pthread_mutex_unlock(&mutex);           // 给缓存队列解锁

        // 以下是处理业务的代码
        printf("phid = %ld, mesgid = %d\n", pthread_self(), stmsg.mesgid);
        usleep(100);
    }
    
}
