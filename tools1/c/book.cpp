#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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

    // 先定义一个指针变量用于存放 shmat 函数的返回地址。（用于指向共享内存的结构体白能量指针）
    struct st_pid* stpid = nullptr;     // 初始化为 0 或者 NULL 或者在c++11中可以初始化为 nullptr

    // 通常来说，只要shmid 共享内存id没有填错，一般shmat是不会失败的，如果失败会返回 -1
    // 注意，这里shmat的返回值需要强制转换成你需要的类型，同时后面的判断条件 -1 也需要强制转换成 （void *)，因为shmat这个函数他的返回值类型是 一个(void *)指针类型
    if((stpid = (struct st_pid*)shmat(shmid, 0, 0)) == (void *)-1)
    {
        printf("shmat(shmid, 0, 0) faild \n");
    }

    // 然后就可以像使用普通的指针一样使用共享内存了

    // 在赋值之前我们先把共享内存里面的东西读出来
    printf("pid = %d， name = %s \n", stpid->pid, stpid->name);

    stpid->pid = getpid();              // 把当前进程的id赋给共享内存的pid成员
    // strcpy(stpid->name, argv[0]);       // 然后把当前进程（程序）的名称赋给共享内存的name成员
    strcpy(stpid->name, argv[1]);       // 然后把当前进程（程序）的第一个参数赋给共享内存的name成员

    // 在赋值之后我们也把共享内存里面的东西读出来
    printf("pid = %d， name = %s \n", stpid->pid, stpid->name);


    // 把共享内存从当前进程中分离
    // shmdt 这个函数他的参数就是上面这里这个指针变量 stpid 指向的内存地址
    // 只要这个指针变量他的内存地址没有问题，那么shmdt这个函数肯定不会失败，所以这里不用判断他的返回值
    shmdt(stpid);

    return 0;
}