#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "_public.h"

// 这里共享内存的key我们使用一个宏来定义，不要将其写死
#define SHMKEYP_    0X5098  // 共享内存的key
// 共享内存的总数和进程数有关，这里我们也用一个宏来表示
#define MAXNUMP_    1000    // 最大进程数量

// 创建一个心跳信息的结构体
struct st_pinfo
{
    int pid;        // 进程ID
    char pname[51]; // 进程名称，可以为空
    int timeout;    // 超时时间，单位：秒
    time_t atime;   // 最后一次心跳时间，用整数表示
};

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf("Using: ./book procname\n");
        return 0;
    }

    // 创建/获取共享内存，大小为 n*sizeof(struc st_pinfo)，这个n：比如说，我们这个程序运行的进程的总数不会超过1000 个，那么这个n就可以取值1000
    int m_shmid = 0;
    // 这里共享内存的key我们使用一个宏来定义，不要将其写死
    // 共享内存的总数和进程数有关，这里我们也用一个宏来表示
    if( (m_shmid = shmget(SHMKEYP_, MAXNUMP_*sizeof(struct st_pinfo), 0640|IPC_CREAT)) == -1 )
    {
        printf("shmget(SHMKEYP_, MAXNUMP_*sizeof(struct st_pinfo), 0640|IPC_CREAT) faild\n");
    }

    // 将共享内存连接到当前进程的地址空间
    struct st_pinfo* m_shm;
    m_shm = (struct st_pinfo*)shmat(m_shmid, 0, 0);

    // 创建当前进程心跳信息结构体变量，把本进程的信息填进去
    struct st_pinfo stpinfo;
    memset(&stpinfo, 0, sizeof(struct st_pinfo));
    stpinfo.pid = getpid();
    STRNCPY(stpinfo.pname, sizeof(stpinfo.pname), argv[1], 50); // STRNCPY自己封装的安全的字符串拷贝函数
    stpinfo.timeout = 30;
    stpinfo.atime = time(0);        // 最后一次心跳时间，用当前时间

    int m_post = -1;

    // 在共享内存中查找一个空位置，把当前进程的心跳信息存入到共享内存中
    for(int i = 0; i < SHMKEYP_; i++)
    {
        // if((m_shm+i)->pid == 0) // 采用地址运算的方法遍历
        if(m_shm[i].pid == 0)   // 采用数组下标的方法来遍历
        {
            // 找到了一个空位置
            m_post = i;
            break;
        }
    }

    // 当然也不排除找不到空位置的情况，这种情况就是进程数实在是太多，已经把共享内存数耗尽了
    if(m_post == -1)
    {
        printf("共享内存空间已经耗尽\n");
        return -1;
    }

    // 找到了这个空位置，把这个结构体拷贝到这个功能内存位置中去
    memcpy(m_shm+m_post, &stpinfo, sizeof(struct st_pinfo));

    while(true)
    {
        // 然后进入循环之后，每个一段时间就更新更新内存中本进程的心跳时间，表示自己是活着的，要维护这个信息
        sleep(10);
    }

    // 如果跳出这个循环，那么要把当前进程从共享内存中移除，每个进程退出的时候，都应该主动的从共享内存中把自己的信息删除掉

    // 把共享内存从当前进程中分离

    return 0;
}