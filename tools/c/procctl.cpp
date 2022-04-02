#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char* argv[])
{
    // 我们要实现的调度程序是这样的
    // /project/tools/bin/project1 60 /project/idc/bin/crtsurfdata /project/idc/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata.log xml,json,cvs

    // 首先这个程序运行至少需要两个参数，时间间隔和被执行（调度）的程序名
    if(argc < 3)
    {
        printf("Using: ./procctl timetvl program argv... \n");
        printf("Example: /project/tools/bin/procctl 5 /usr/bin/ls -lt /log/idc\n");

        printf("本程序是服务程序的调度程序，周期性启动服务程序或shell脚本。\n");
        printf("timetvl 运行周期，单位：秒。被调度的程序运行结束后，在timetvl秒后会被procctl重新启动。\n");
        printf("program 被调度的程序名，必须使用全路径。\n");
        printf("argvs   被调度的程序的参数。\n");
        printf("注意，本程序不会被kill杀死，但可以用kill -9强行杀死。\n\n\n");

        return -1;
    }

    // 要实现程序调度，就可以先执行fork函数，创建一个子进程，然后让子进程调用execl执行新的程序，
    // 那么新程序就将替换子进程，不会影响到父进程。
    // 在父进程中可以调用wait等函数等待新进程（程序）运行结果，这样就实现了程序调度功能。

    // 服务程序一般需要关闭全部的信号，是因为服务程序不希望被信号干扰
    // 同时还要关闭IO，关闭IO是因为这个程序要运行在后台，不需要IO
    for(int ii = 0; ii < 64; ii++)
    {
        // 忽略信号
        signal(ii, SIG_IGN);
        // 关闭io
        close(ii);
    }

    // 让程序运行在后台
    // 生成子进程，父进程退出，让程序运行在后台，由系统1号进程托管
    if(fork() != 0) exit(0);

    // 启用子进程退出的信号
    signal(SIGCHLD, SIG_DFL);

    char* pargv[argc];
    for(int ii = 2; ii < argc; ii++)
    {
        pargv[ii-2] = argv[ii];
    }

    pargv[argc - 2] = NULL;

    while(true)
    {
        if(fork() == 0)
        {
            // execl("/usr/bin/ls", "/usr/bin/ls", "-lt", "/log/idc", (char*)0);
            
            execv(argv[2], pargv);
            
            // 这行代码在 execv 成功的时候，不会被执行，但是在execv失败的时候这行代码就会被执行
            // 然后子进程退出
            exit(0);
            // ....
        }
        else
        {
            int status;
            wait(&status);
            sleep(atoi(argv[1]));
        }
    }

    return 0;
}