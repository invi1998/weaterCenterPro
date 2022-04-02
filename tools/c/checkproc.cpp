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

        printf("Example:/project/tools/bin/procctl 10 /project/tools/bin/checkproc /tmp/log/checkproc.log\n\n");

        printf("本程序用于检查后台服务程序是否超时，如果已超时，就终止它。\n");
        printf("注意：\n");
        printf("  1）本程序由procctl启动，运行周期建议为10秒。\n");
        printf("  2）为了避免被普通用户误杀，本程序应该用root用户启动。\n");
        printf("  3）如果要停止本程序，只能用killall -9 终止。\n\n\n");

        return 0;
    }

    // 服务程序应该忽略所有的信号和关闭IO，只保留自己关系的信号，不希望程序被干扰
    // 这里关闭信号和io（这里只能关闭标准输入，标准输出和标准错误这3个io)
    CloseIOAndSignal(true);

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
        // 程序稳定后这行日志也可以不用写了，日志只记录关键信息
        // logfile.Write("i = %d, pid = %d, pname = %s, timeout = %d, atime = %ld\n", i, shm[i].pid, shm[i].pname, shm[i].timeout, shm[i].atime);

        // 然后向进程发送信号0，判断它是否还存在，如果不存在，把共享内存中删除该记录，continue
        // kill这个函数，它向进程发送信号，如果进程存在，就返回0，如果进程不存在就返回-1
        int iret = kill(shm[i].pid, 0);
        if(iret == -1)
        {
            // 进程不存在，先记录一个日志
            logfile.Write("进程pid=%d(%s)已经不存在\n", shm[i].pid, shm[i].pname);

            // 然后把结构体清零(将该进程的心跳信息从共享内存中删除)
            memset(shm+i, 0, sizeof(struct st_procinfo));
            continue;
        }

        // 然后再来判断进程有没有超时，如果没有超时，continue
        time_t now = time(0);   // 获取当前时间
        if(now-shm[i].atime < shm[i].timeout) continue;

        // 如果进程心跳超时，先向进程发送15的信号，尝试正常终止进程
        logfile.Write("进程pid=%d(%s)心跳超时\n", (shm+i)->pid, (shm+i)->pname);

        kill(shm[i].pid, 15);       // 发送信号15，尝试正常终止进程

        // 服务程序在接收到15的这个信号之后，会正常退出，但是这个退出需要一些时间，所以下面我用一个循环来检测服务程序是否退出
        for(int j = 0; j < 5; j++)
        {
            sleep(1);
            iret = kill(shm[i].pid, 0);     // 向进程发送信号0，判断它是否存在
            if(iret == -1) break;   // 进程已经退出
        }

        // 如果15不能终止，进程依然存在，就发送信号9，强制终止它
        if(iret == -1)
        {
            logfile.Write("进程pid=%d(%s)已经正常退出\n", shm[i].pid, shm[i].pname);
        }
        else
        {
            kill(shm[i].pid, 9);
            logfile.Write("进程pid=%d(%s)已经正常退出\n", shm[i].pid, shm[i].pname);
        }

        // 从共享内存中删除已经心跳超时的进程的心跳记录
        memset(shm+i, 0, sizeof(struct st_procinfo));   // 从共享内存中删除该记录
    }

    // 共享内存中全部记录被处理完之后，就把共享内存从当前进程中分离，程序退出
    shmdt(shm);

    return 0;
}