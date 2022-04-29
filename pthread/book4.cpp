// 线程参数传递
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

void * thmain1(void * arg);

int var = 0;

struct st_args
{
    int no;     // 线程编号
    char name[51];  // 线程名
};


int main(int argc, char* argv[])
{

    pthread_t thid1=0;

    // 创建线程
    struct st_args *stargs = new st_args;
    stargs->no = 10;
    strcpy(stargs->name, "测试线程");
    if(pthread_create(&thid1, NULL, thmain1, stargs) != 0)
    {
        printf("线程创建失败\n");
        exit(-1);
    }

    // 等待子线程退出
    printf("join...\n");
    pthread_join(thid1, NULL);
    printf("join-ok\n");
}

void * thmain1(void * arg)
{
    struct st_args* pst = (struct st_args*)arg;

    printf("no = %d\nname = %s\n", pst->no, pst->name);
    
    delete pst;
    return NULL;
}
