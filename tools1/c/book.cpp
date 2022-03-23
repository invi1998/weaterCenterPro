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

    return 0;
}