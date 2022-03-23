
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


int main()
{

    int pid = fork();

    if(pid == 0)
    {
        printf("这里是子进程，进程id = %d，将执行子进程的任务。\n", getpid());
        sleep(20);
    }

    if(pid > 0)
    {
        printf("这里是父进程，进程id = %d，将执行父进程的任务。\n", getpid());

        sleep(10);
    }

    return 0;
}