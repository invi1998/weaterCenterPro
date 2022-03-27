// 守护进程
#include "_public.h"

// 程序运行的日志文件
CLogFile logfile;

int main(int argc, char* argv[])
{

    // 编写程序的帮助文档
    if (argc != 2)
    {
        printf("\n");
        printf("Using:./checkproc logfilename\n");

        printf("Example:/project/tools1/bin/procctl 10 /project/tools1/bin/checkproc /tmp/log/checkproc.log\n\n");

        printf("本程序用于检查后台服务程序是否超时，如果已超时，就终止它。\n");
        printf("注意：\n");
        printf("  1）本程序由procctl启动，运行周期建议为10秒。\n");
        printf("  2）为了避免被普通用户误杀，本程序应该用root用户启动。\n");
        printf("  3）如果要停止本程序，只能用killall -9 终止。\n\n\n");

        return 0;
    }

    // 打开日志文件
    if(logfile.Open(argv[1], "a+") == false)
    {
        printf("logfile.Open(argv[1], \"a+\") faild \n");
        return -1;
    }

    // 创建、获取共享内存（共享内存可以由守护进程创建，也可以由后台服务程序创建）
    // 共享内存键值为SHMKEYP，大小为MAXNUMP个st_procinfo结构体大小
    int shmid = 0;
    if( (shmid = shmget((key_t)SHMKEYP, MAXNUMP*sizeof(struct st_procinfo), 0666|IPC_CREAT)) == -1)
    {
        logfile.Write("创建、获取共享内存（%x）失败\n", SHMKEYP);
        return -1;
    }

    // 将共享内存连接到当前进程的地址空间
    struct st_procinfo* shm = (struct st_procinfo*)shmat(shmid, 0, 0);

    // 用一个for循环遍历共享内存中的全部记录
    for(int i = 0; i< MAXNUMP; i++)
    {
        // 如果记录的进程编号等于0（pid == 0） 表示是一条空记录，continue
        if(shm[i].pid == 0) continue;

        // 如果记录的进程编号不等于0（pid != 0），表示是一个进程（服务程序）的心跳记录
        logfile.Write("i = %d, pid = %d, pname = %s, timeout = %d, atime = %ld\n", i, shm[i].pid, shm[i].pname, shm[i].timeout, shm[i].atime);

        // 然后向进程发送信号0，判断它是否还存在，如果不存在，把共享内存中删除该记录，continue

        // 然后再来判断进程有没有超时，如果没有超时，continue

        // 如果进程心跳超时，先向进程发送15的信号，尝试正常终止进程

        // 如果15不能终止，进程依然存在，就发送信号9，强制终止它

        // 从共享内存中删除已经心跳超时的进程的心跳记录
    }

    // 共享内存中全部记录被处理完之后，就把共享内存从当前进程中分离，程序退出

    return 0;
}