// obtmindtodb.cpp，本程序用于把全国站点分钟观测数据入库到T_ZHOBTMIND表中，支持xml和csv两种文件格式。

#include "idcapp.h"

CLogFile logfile;

connection conn;

CPActive PActive;

void EXIT(int sig);

// 业务处理主函数
bool _obtmindtodb(const char* pathname, char* connstr, char* charset);

int main(int argc, char* argv[])
{

    // 帮助文档。
    if (argc!=5)
    {
        printf("\n");
        printf("Using:./obtmindtodb pathname connstr charset logfile\n");

        printf("Example:/project/tools/bin/procctl 10 /project/idc/bin/obtmindtodb /idcdata/surfdata \"192.168.31.133,root,password,mysql,3306\" utf8 /log/idc/obtmindtodb.log\n\n");

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

    PActive.AddPInfo(30, "obtmindtodb");        // 进程的心跳时间10s

    // 业务处理主函数
    _obtmindtodb(argv[1], argv[2], argv[3]);

    return 0;
}

void EXIT(int sig)
{
  logfile.Write("程序退出，sig=%d\n\n",sig);

  conn.disconnect();

  exit(0);
}

// 业务处理主函数
bool _obtmindtodb(const char* pathname, char* connstr, char* charset)
{
    CDir Dir;
    // 打开目录
    if(Dir.OpenDir(pathname, "*.xml,*csv") == false)
    {
        logfile.Write("Dir.OpenDir(%s, \"*.xml\") failed\n", pathname);
        return false;
    }

    CFile File;

    CZHOBTMIND zhobtmindObj(&conn, &logfile);

    int totalcount = 0;         // 文件的总记录数
    int insertcount = 0;        // 成功插入的记录数
    CTimer Timer;               // 计时器，记录每个数据文件的处理耗时
    int fileType = -1;              // 文件类型， 0 - xml; 1 - csv

    while (true)
    {
        // 读取目录，得到一个数据文件名
        if(Dir.ReadDir() == false)
        {
            break;
        }

        if(MatchStr(Dir.m_FileName, "*.xml") == true)
        {
            fileType = 0;
        }
        else if(MatchStr(Dir.m_FileName, "*.csv") == true)
        {
            fileType = 1;
        }
        
        // 判断数据库是否已经连接，如果已经连接，就不用再连数据库了(m_state与数据库的连接状态，0-未连接，1-已连接)
        if(conn.m_state == 0)
        {
            // 连接数据库
            if(conn.connecttodb(connstr, charset) != 0)
            {
                logfile.Write("connect database(%s) failed\n%s\n", connstr, conn.m_cda.message);
                return false;
            }
            logfile.Write("connect database(%s) ok\n", connstr);

        }

        totalcount, insertcount = 0;

        // 打开文件

        if(File.Open(Dir.m_FullFileName, "r") == false)
        {
            logfile.Write("File.Open(%s, \"r\")", Dir.m_FullFileName);
            return false;
        }

        // 存放从文件中读取的一行
        char strbuffer[1001];

        /*  表结构
        +-----------+-------------+------+-----+-------------------+-----------------------------+
        | Field     | Type        | Null | Key | Default           | Extra                       |
        +-----------+-------------+------+-----+-------------------+-----------------------------+
        | obtid     | varchar(10) | NO   | PRI | NULL              |                             |
        | ddatetime | datetime    | NO   | PRI | NULL              |                             |
        | t         | int(11)     | YES  |     | NULL              |                             |
        | p         | int(11)     | YES  |     | NULL              |                             |
        | u         | int(11)     | YES  |     | NULL              |                             |
        | wd        | int(11)     | YES  |     | NULL              |                             |
        | wf        | int(11)     | YES  |     | NULL              |                             |
        | r         | int(11)     | YES  |     | NULL              |                             |
        | vis       | int(11)     | YES  |     | NULL              |                             |
        | upttime   | timestamp   | NO   |     | CURRENT_TIMESTAMP | on update CURRENT_TIMESTAMP |
        | keyid     | bigint(20)  | NO   | UNI | NULL              | auto_increment              |
        +-----------+-------------+------+-----+-------------------+-----------------------------+
        */

        while (1)
        {
            // 处理文件中的每一行
            if(fileType == 0)
            {
                // 读取文件的一行到strbuffer中，其中以 “</endl>”为字符串结束标志（为行读取结束标志）
                if(File.FFGETS(strbuffer, 1000, "<endl/>") == false) break;
            }
            else if (fileType == 1)
            {
                // 读取文件的一行到strbuffer中，其中以 “</endl>”为字符串结束标志（为行读取结束标志）
                if(File.Fgets(strbuffer, 1000, true) == false) break;
                if (strstr(strbuffer, "站点") != 0) continue;       // 扔掉文件中的第一行
            }
            

            totalcount++;

            zhobtmindObj.SplitBuffer(strbuffer, fileType);

            if(zhobtmindObj.InsertTable() == true) insertcount++;

        }

        // 删除文件，提交事务
        File.CloseAndRemove();
        conn.commit();

        logfile.Write("已处理文件数%s(totalcount = %d, insertcount=%d), 耗时%.2f秒\n", Dir.m_FullFileName, totalcount, insertcount, Timer.Elapsed());

    }
    
    return true;
}