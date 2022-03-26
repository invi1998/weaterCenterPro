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
#define SEMKEYP_    0X5098  // 信号量key

// 创建一个心跳信息的结构体
struct st_pinfo
{
    int pid;        // 进程ID
    char pname[51]; // 进程名称，可以为空
    int timeout;    // 超时时间，单位：秒
    time_t atime;   // 最后一次心跳时间，用整数表示
};

class PActive
{
private:
    CSEM m_sem;             // 用于给共享内存加锁的信号量ID
    int m_shmid;            // 共享内存id
    int m_pos;              // 当前进程在共享内存中的地址空间（或者下标）
    struct st_pinfo *m_shm; // 指向共享内存的地址空间

public:
    PActive();              // 初始化成员变量
    bool AddPInfo(const int timeout, const char* pname);        // 把当前进程的心跳信息写入到共享内存中
    bool UptATime();        // 更新共享内存中当前进程的心跳时间
    ~PActive();             // 从共享内存中删除当前进程的心跳记录
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

    CSEM m_sem;
    if((m_sem.init(SEMKEYP_)) == false)
    {
        printf("m_sem.init(SEMKEYP_) faild\n");
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

    // 进程id是循环使用的，如果曾经有一个很进程异常退出，没有清理自己的心跳信息
    // 他的进程信息就将残留在共享内存中，不巧的是，当前进程重用了上述进程的id
    // 这样就会在共享内存中存在两个进程id相同的记录，守护进程检查到残留进程心跳时
    // 会想进程id发送退出信号，这个信号将会误杀当前进程

    // 如果共享内存中存在当前进程编号，一定是其他进程残留的数据，当前进程就重用该位置即可
    for(int i = 0; i < SHMKEYP_; i++)
    {
        if(m_shm[i].pid == stpinfo.pid)
        {
            m_post = i;
            break;
        }
    }

    // 信号量加锁
    m_sem.P();      // 加锁

    // 这里只有在上面没找到相同进程id的情况下，才进行空位置寻找
    // 在共享内存中查找一个空位置，把当前进程的心跳信息存入到共享内存中
    if(m_post == -1)
    {
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
    }

    // 当然也不排除找不到空位置的情况，这种情况就是进程数实在是太多，已经把共享内存数耗尽了
    if(m_post == -1)
    {
        // 这里异常退出，如果是走到这个分支，不要忘记把这里解锁了
        m_sem.V();
        printf("共享内存空间已经耗尽\n");
        return -1;
    }

    // 找到了这个空位置，把这个结构体拷贝到这个功能内存位置中去
    memcpy(m_shm+m_post, &stpinfo, sizeof(struct st_pinfo));

    // 信号量解锁
    m_sem.V();

    while(true)
    {
        // 然后进入循环之后，每个一段时间就更新更新内存中本进程的心跳时间，表示自己是活着的，要维护这个信息
        m_shm[m_post].atime = time(0);
        sleep(10);
    }

    // 如果跳出这个循环，那么要把当前进程从共享内存中移除，每个进程退出的时候，都应该主动的从共享内存中把自己的信息删除掉
    m_shm[m_post].pid = 0;      // 移除方法其实就是把进程id设置为0，因为我们是通过判断进程id是否为0来判断是否是空位置的
    memset(m_shm+m_post, 0, sizeof(struct st_pinfo));  // 当然更稳妥的方法是加上把这个结构体置0的操作，不过影响不大

    // 把共享内存从当前进程中分离
    shmdt(m_shm);

    return 0;
}


// 初始化成员变量
PActive::PActive()
{
    m_shmid = -1;       // 共享内存id
    m_pos = -1;         // 当前进程在共享内存组中的位置
    m_shm = 0;          // 指向共享内存的地址空间（首地址）
}

// 把当前进程的心跳信息加入共享内存中
bool PActive::AddPInfo(const int timeout, const char* pname)
{

}

// 更新共享内存中当前进程的心跳时间
bool PActive::UptATime()
{

}

// 从共享内存中删除当前进程的心跳记录
PActive::~PActive()
{
    
}