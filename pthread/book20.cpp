#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <iostream>
#include <vector>

sem_t notify;       // 声明用于通知的信号量
sem_t lock;     // 声明用于加锁的信号量

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

    sem_init(&notify, 0, 0);        // （通知用）初始化信号量，第三个参数（信号量的值）设置为0，表示初始时没有资源，让消费者线程等待
    sem_init(&lock, 0, 1);       // （加锁用）初始化信号量，第三个参数为1，表示有锁可以用

    // 创建3个消费者线程
    pthread_t thid1,thid2,thid3;
    pthread_create(&thid1, NULL, outcache, NULL);
    pthread_create(&thid2, NULL, outcache, NULL);
    pthread_create(&thid3, NULL, outcache, NULL);

    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    pthread_join(thid3, NULL);

    sem_destroy(&notify);
    sem_destroy(&lock);
}

void incache(int sig)          // 生产者（数据入队列）
{
    static int mesgid = 1;      // 消息计数器
    struct st_message stmesg;       // 消息内容
    memset(&stmesg, 0, sizeof(struct st_message));

    sem_wait(&lock);

    // 生产数据，放入缓存队列
    // 生产出数据之后，给信号量的值加1
    stmesg.mesgid = mesgid++;vcache.push_back(stmesg);
    stmesg.mesgid = mesgid++;vcache.push_back(stmesg);
    stmesg.mesgid = mesgid++;vcache.push_back(stmesg);
    stmesg.mesgid = mesgid++;vcache.push_back(stmesg);
    stmesg.mesgid = mesgid++;vcache.push_back(stmesg);

    sem_post(&lock);

    sem_post(&notify);
    sem_post(&notify);
    sem_post(&notify);
    sem_post(&notify);
    sem_post(&notify);
}

void *outcache(void* arg)      // 消费者（数据出队列线程主函数）
{
    struct st_message stmsg;

    while (true)
    {
        sem_wait(&lock);     // 给缓存队列加锁
        // if(vcache.size() == 0)
        // 如果缓存队列为空，等待，用while防止条件变量被虚假唤醒
        while(vcache.size() == 0)
        {
            // 信号量没有给互斥锁加锁和解锁的功能，需要手动进行加锁
            sem_post(&lock);                // 给缓存队列解锁
            sem_wait(&notify);              // 等待信号量的值大于0
            sem_wait(&lock);                // 给缓存队列加锁
        }

        // 从缓存对垒中获取第一条激励，然后删除该记录
        memcpy(&stmsg, &vcache[0], sizeof(struct st_message));     // 内存拷贝

        vcache.erase(vcache.begin());

        sem_post(&lock);           // 给缓存队列解锁

        // 以下是处理业务的代码
        printf("phid = %ld, mesgid = %d\n", pthread_self(), stmsg.mesgid);
        usleep(100);
    }
}

