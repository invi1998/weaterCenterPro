#include "_public.h"

// 程序退出和信号2,15的处理函数
void EXIT(int sig);

int main(int argc, char* argv[])
{
    // 程序的帮助
    if(argc != 4)
    {
        printf("\n");
        printf("Using:/project/tools/bin/gzipfiles pathname matchstr timeout\n\n");

        printf("Example:/project/tools/bin/gzipfiles /log/idc \"*.log.20*\" 0.02\n");
        printf("        /project/tools/bin/gzipfiles /tmp/idc/surfdata \"*.xml,*.json\" 0.01\n");
        printf("        /project/tools/bin/procctl 300 /project/tools/bin/gzipfiles /log/idc \"*.log.20*\" 0.02\n");
        printf("        /project/tools/bin/procctl 300 /project/tools/bin/gzipfiles /tmp/idc/surfdata \"*.xml,*.json\" 0.01\n\n");

        printf("这是一个工具程序，用于压缩历史的数据文件或日志文件。\n");
        printf("本程序把pathname目录及子目录中timeout天之前的匹配matchstr文件全部压缩，timeout可以是小数。\n");
        printf("本程序不写日志文件，也不会在控制台输出任何信息。\n");
        printf("本程序调用/usr/bin/gzip命令压缩文件。\n\n\n");

        return -1;
    }

    // 关闭全部信号和io(输入输出)                                                                                       
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
    // 但请不要用 "kill -9 +进程号" 强行终止。
    CloseIOAndSignal(true);
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    // 获取文件超时的时间点（本程序要压缩历史文件，历史文件怎么定义？）
    // 这里可以认为一个小时之前的文件属于历史文件，或者认为3天之前的属于历史文件，这里总有一个时间点，这里我们就获取这个时间点
    // 这个时间点，应该是由main函数的参数指定的
    char strTimeOut[21];
    LocalTime(strTimeOut, "yyyy-mm-dd hh24:mi:ss", 0 - (int)(atof(argv[3])*24*60*60));

    // 打开目录，CDir.OpenDir()。打开目录可以使用开发框架里这个CDir类的OpenDir方法来打开目录
    CDir Dir;
    // 打开文件的目录，打开文件的匹配规则，最多打开10000个文件，需要遍历子目录，对文件不排序
    if(Dir.OpenDir(argv[1], argv[2], 10000, true, false) == false)
    {
        printf("Dir.OpenDir(%s, %s, 10000, true, false) failed \n", argv[1], argv[2]);
        return -1;
    }

    char strCmd[1024];      // 存放 gzip 压缩文件的命令
    // 然后就是遍历目录中的文件名
    while(true)
    {
        // 在循环中调用 CDir.ReadDir()方法读取每个文件的信息（包括文件名，文件时间）
        if(Dir.ReadDir() == false)
        {
            // 读取失败，表示没有文件了（读完了）
            break;
        }

        // printf("DirName=%s, FileName=%s, FullFileName=%s, FileSize=%d, ModifyTime=%s, CreateTime=%s, AccessTime=%s\n",\
        //         Dir.m_DirName, Dir.m_FileName, Dir.m_FullFileName, Dir.m_FileSize, Dir.m_ModifyTime, Dir.m_CreateTime, Dir.m_AccessTime);

        // 然后把文件的时间和超时时间点做比较，如果文件时间更早，就说明是一个历史文件，就需要进行压缩
        // 注意：这里不仅要和时间点做比较，还要和文件名做比（通过文件名来判断是否是已经被压缩过的文件，压缩过的文件就需要进行压缩了）
        // MatchStr 是框架里的一个用于比较字符串是否匹配的函数
        if((strcmp(Dir.m_ModifyTime, strTimeOut)) < 0 && (MatchStr(Dir.m_FileName, "*.gz") == false))
        {
            // 压缩文件，调用操作系统的gzip命令压缩文件
            // SNPRINTF 函数是框架里的安全函数，功能和snprintf是一样的（将可变参数 “…” 按照format的格式格式化为字符串，然后再将其拷贝至str中。）
            // 1>/dev/null 2>/dev/null 这两各参数的写法是我们经常的写法，就是将标准输出和标准错误都定位为空（黑洞），也就是不用输出任何东西
            SNPRINTF(strCmd, sizeof(strCmd), 1000, "/usr/bin/gzip -f %s 1>/dev/null 2>/dev/null", Dir.m_FullFileName);
            if(system(strCmd) == 0)
            {
                printf("gzip %s ok.\n", Dir.m_FullFileName);
            }
            else
            {
                printf("gzip %s failed.\n", Dir.m_FullFileName);
            }
        }

    }

    return 0;
}

void EXIT(int sig)
{
    printf("程序退出， sig = %d\n\n", sig);

    exit(0);
}