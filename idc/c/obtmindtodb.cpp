// obtmindtodb.cpp，本程序用于把全国站点分钟观测数据入库到T_ZHOBTMIND表中，支持xml和csv两种文件格式。

#include "_public.h"
#include "_mysql.h"

CLogFile logfile;

connection conn;

CPActive PActive;

void EXIT(int sig);

int main(int argc, char* argv[])
{

    // 帮助文档。
    if (argc!=5)
    {
        printf("\n");
        printf("Using:./obtmindtodb pathname connstr charset logfile\n");

        printf("Example:/project/tools/bin/procctl 10 /project/idc/bin/obtmindtodb /idcdata/surfdata \"192.168.31.133,root,sh269jgl105,mysql,3306\" utf8 /log/idc/obtmindtodb.log\n\n");

        printf("本程序用于把全国站点分钟观测数据保存到数据库的T_ZHOBTMIND表中，数据只插入，不更新。\n");
        printf("pathname 全国站点分钟观测数据文件存放的目录。\n");
        printf("connstr  数据库连接参数：ip,username,password,dbname,port\n");
        printf("charset  数据库的字符集。\n");
        printf("logfile  本程序运行的日志文件名。\n");
        printf("程序每10秒运行一次，由procctl调度。\n\n\n");

        return -1;
    }

    // 处理程序退出信号
    // 关闭全部的信号和输入输出。
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
    // 但请不要用 "kill -9 +进程号" 强行终止。
    CloseIOAndSignal(); signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    // 打开日志文件。
    if (logfile.Open(argv[4],"a+")==false)
    {
        printf("打开日志文件失败（%s）。\n",argv[4]);
        return -1;
    }

    PActive.AddPInfo(10, "obtcodetodb");        // 进程的心跳时间10s

    return 0;
}

void EXIT(int sig)
{
  logfile.Write("程序退出，sig=%d\n\n",sig);

  conn.disconnect();

  exit(0);
}