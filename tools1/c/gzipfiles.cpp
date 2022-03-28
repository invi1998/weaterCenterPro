#include "_public.cpp"

// 程序退出和信号2,15的处理函数
void EXIT(int sig);

int main(int argc, char* argv[])
{
    // 程序的帮助
    if(argc != 4)
    {
        printf("\n");
        printf("Using:/project/tools1/bin/gzipfiles pathname matchstr timeout\n\n");

        printf("Example:/project/tools1/bin/gzipfiles /log/idc \"*.log.20*\" 0.02\n");
        printf("        /project/tools1/bin/gzipfiles /tmp/idc/surfdata \"*.xml,*.json\" 0.01\n");
        printf("        /project/tools1/bin/procctl 300 /project/tools1/bin/gzipfiles /log/idc \"*.log.20*\" 0.02\n");
        printf("        /project/tools1/bin/procctl 300 /project/tools1/bin/gzipfiles /tmp/idc/surfdata \"*.xml,*.json\" 0.01\n\n");

        printf("这是一个工具程序，用于压缩历史的数据文件或日志文件。\n");
        printf("本程序把pathname目录及子目录中timeout天之前的匹配matchstr文件全部压缩，timeout可以是小数。\n");
        printf("本程序不写日志文件，也不会在控制台输出任何信息。\n");
        printf("本程序调用/usr/bin/gzip命令压缩文件。\n\n\n");

        return -1;
    }

    // 关闭全部信号和io(输入输出)                                                                                       
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
    // 但请不要用 "kill -9 +进程号" 强行终止。
    // CloseIOAndSignal(true);
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    // 获取文件超时的时间点（本程序要压缩历史文件，历史文件怎么定义？）
    // 这里可以认为一个小时之前的文件属于历史文件，或者认为3天之前的属于历史文件，这里总有一个时间点，这里我们就获取这个时间点
    // 这个时间点，应该是由main函数的参数指定的
    char strTimeOut[21];
    LocalTime(strTimeOut, "yyyy-mm-dd hh:24:ss", atof(argv[3]))

    // 打开目录，CDir.OpenDir()。打开目录可以使用开发框架里这个CDir类的OpenDir方法来打开目录

    // 然后就是遍历目录中的文件名
    while(true)
    {
        // 在循环中调用 CDir.ReadDir()方法读取每个文件的信息（包括文件名，文件时间）

        // 然后把文件的时间和超时时间点做比较，如果文件时间更早，就说明是一个历史文件，就需要进行压缩

        // 压缩文件，调用操作系统的gzip命令压缩文件
    }

    return 0;
}

void EXIT(int sig)
{
    printf("程序退出， sig = %d\n\n", sig);

    exit();
}