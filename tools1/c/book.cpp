#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "_public.h"

// 创建一个信号量对象，用于给共享内存加锁
CSEM sem;

struct st_pid
{
    int pid;        // 进程编号
    char name[51];  // 进程名称
};

int main(int argc, char* argv[])
{
    // 共享内存标志
    int shmid;

    // 这个共享内存，第一个参数
    // 第二个参数，这里我打算用这个共享内存来保存一个结构体，所以这里内存大小设置为结构体的size值
    // 然后第三个参数，我们一般这么写 0640|IPC_CREAT 
    // 它包括两个部分，前面这部分是操作权限，他的表示方法跟文件和目录的表示方法是一样的（八进制）
    // 第二部分（IPC_CREAT）表示如果共享内存已经存在，就获得他的id，如果不存在就创建它（这部分参数基本上没有选择，只能填这个）
    // shmget函数，成功返回共享内存的id，失败返回-1，系统错误变量并设置，一般来说，如果不是因为系统内存不够，很少会返回失败
    if((shmid = shmget(0x5005, sizeof(struct st_pid), 0640|IPC_CREAT)) == -1)
    {
        printf("shmget(0x5005, sizeof(struct st_pid), 0640|IPC_CREAT) faild \n");
    }

    // 初始化信号量，第一个参数key：是和信号量那个key是一样的，也是一个十六进制数，注意这里这个key也可以使用共享内存那个key,他们两个是独立的，互不冲突
    // 第二个参数：value 表示信号量的初始值，而知信号量的初始值缺省为1
    // 第三个参数，信号量的标志 如果是二值信号量的化就是 SEM_UNDO 也是缺省的 
    // （所以可以看出，我们提供的这个CSEM 信号量类，默认情况下是一个二值信号量）
    // 如果信号量已经存在，获取信号信号量，如果不存在，则创建它并初始化为value
    if(sem.init(0x5005) == false)
    {
        printf("sem.init(0x5005) faild \n");
        return -1;
    }

    // 然后编写加锁和解锁的代码

    // 先定义一个指针变量用于存放 shmat 函数的返回地址。（用于指向共享内存的结构体白能量指针）
    struct st_pid* stpid = nullptr;     // 初始化为 0 或者 NULL 或者在c++11中可以初始化为 nullptr

    // 通常来说，只要shmid 共享内存id没有填错，一般shmat是不会失败的，如果失败会返回 -1
    // 注意，这里shmat的返回值需要强制转换成你需要的类型，同时后面的判断条件 -1 也需要强制转换成 （void *)，因为shmat这个函数他的返回值类型是 一个(void *)指针类型
    if((stpid = (struct st_pid*)shmat(shmid, 0, 0)) == (void *)-1)
    {
        printf("shmat(shmid, 0, 0) faild \n");
    }
    // 然后就可以像使用普通的指针一样使用共享内存了

    // 在加锁之前将当前时间 和 信号量的值显示出来
    printf("start  time = %ld， value = %d \n", time(0), sem.value());

    sem.P();        // 加锁

    // 在加锁之后将当前时间 和 信号量的值显示出来
    printf("start  time = %ld， value = %d \n", time(0), sem.value());

    // 在赋值之前我们先把共享内存里面的东西读出来
    printf("pid = %d， name = %s \n", stpid->pid, stpid->name);

    stpid->pid = getpid();              // 把当前进程的id赋给共享内存的pid成员
    // strcpy(stpid->name, argv[0]);       // 然后把当前进程（程序）的名称赋给共享内存的name成员
    strcpy(stpid->name, argv[1]);       // 然后把当前进程（程序）的第一个参数赋给共享内存的name成员

    // 在赋值之后我们也把共享内存里面的东西读出来
    printf("pid = %d， name = %s \n", stpid->pid, stpid->name);
    sleep(10);

    // 在解锁之前将当前时间 和 信号量的值显示出来
    printf("end  time = %ld， value = %d \n", time(0), sem.value());

    sem.V();        // 解锁

    // 在解锁之后将当前时间 和 信号量的值显示出来
    printf("end  time = %ld， value = %d \n", time(0), sem.value());


    // 把共享内存从当前进程中分离
    // shmdt 这个函数他的参数就是上面这里这个指针变量 stpid 指向的内存地址
    // 只要这个指针变量他的内存地址没有问题，那么shmdt这个函数肯定不会失败，所以这里不用判断他的返回值
    shmdt(stpid);

    // 删除共享内存
    // 如果删除失败，返回-1
    if(shmctl(shmid, IPC_RMID, 0) == -1)
    {
        printf("shmctl(stpid, IPC_RMID, 0) faild \n");
    }

    return 0;
}