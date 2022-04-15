// obtmindtodb.cpp，本程序用于把全国站点分钟观测数据入库到T_ZHOBTMIND表中，支持xml和csv两种文件格式。

#include "_public.h"
#include "_mysql.h"

CLogFile logfile;

connection conn;

CPActive PActive;

struct st_zhobtmind
{
  char obtid[11];      // 站点代码。
  char ddatetime[21];  // 数据时间，精确到分钟。
  char t[11];          // 温度，单位：0.1摄氏度。
  char p[11];          // 气压，单位：0.1百帕。
  char u[11];          // 相对湿度，0-100之间的值。
  char wd[11];         // 风向，0-360之间的值。
  char wf[11];         // 风速：单位0.1m/s。
  char r[11];          // 降雨量：0.1mm。
  char vis[11];        // 能见度：0.1米。
}stzhobtmind;


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

    PActive.AddPInfo(5000, "obtmindtodb");        // 进程的心跳时间10s

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
    sqlstatement stmt;      // 数据库连接对象
    CDir Dir;
    // 打开目录
    if(Dir.OpenDir(pathname, "*.xml") == false)
    {
        logfile.Write("Dir.OpenDir(%s, \"*.xml\") failed\n", pathname);
        return false;
    }

    CFile File;

    int totalcount = 0;         // 文件的总记录数
    int insertcount = 0;        // 成功插入的记录数
    CTimer Timer;               // 计时器，记录每个数据文件的处理耗时

    while (true)
    {
        // 读取目录，得到一个数据文件名
        if(Dir.ReadDir() == false)
        {
            break;
        }
        logfile.Write("filename = %s\n", Dir.m_FullFileName);
        
        // 判断数据库是否已经连接，如果已经连接，就不用再连数据库了(m_state与数据库的连接状态，0-未连接，1-已连接)
        if(conn.m_state == 0)
        {
            // 连接数据库
            if(conn.connecttodb(connstr, charset) != 0)
            {
                logfile.Write("connect database(%s) failed\n%s\n", connstr, conn.m_cda.message);
                return -1;
            }
            logfile.Write("connect database(%s) ok\n", connstr);

        }

        // 只有当stmt对象没有绑定数据库对象的时候，才进行绑定(与数据库连接的绑定状态，0-未绑定，1-已绑定)
        if(stmt.m_state == 0)
        {
            stmt.connect(&conn);
            stmt.prepare("insert into T_ZHOBTMIND(obtid, ddatetime, t, p, u, wd, wf, r, vis)\
            values(:1, str_to_date(:2, '%%Y%%m%%d%%H%%i%%s'), :3, :4, :5, :6, :7, :8, :9)");
            
            // 绑定参数
            stmt.bindin(1, stzhobtmind.obtid, 10);
            stmt.bindin(2, stzhobtmind.ddatetime, 14);
            stmt.bindin(3,stzhobtmind.t,10);
            stmt.bindin(4,stzhobtmind.p,10);
            stmt.bindin(5,stzhobtmind.u,10);
            stmt.bindin(6,stzhobtmind.wd,10);
            stmt.bindin(7,stzhobtmind.wf,10);
            stmt.bindin(8,stzhobtmind.r,10);
            stmt.bindin(9,stzhobtmind.vis,10);
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
            // 读取文件的一行到strbuffer中，其中以 “</endl>”为字符串结束标志（为行读取结束标志）
            if(File.FFGETS(strbuffer, 1000, "<endl/>") == false)
            {
                logfile.Write("File.FFGETS(strbuffer, 1000, \"<endl/>\") failed\n");
                break;
            }

            totalcount++;

            // 初始化结构体
            memset(&stzhobtmind, 0, sizeof(struct st_zhobtmind));
            GetXMLBuffer(strbuffer, "obtid", stzhobtmind.obtid, 10);
            GetXMLBuffer(strbuffer, "ddatetime", stzhobtmind.ddatetime, 14);
            char tmp[11];       // 定义一个临时变量
            GetXMLBuffer(strbuffer, "t", tmp, 10);      // xml解析出来的内容放到临时变量tmp中
            if(strlen(tmp) > 0)     // 如果取出来的临时变量不为空（表示有数据）
            {
                // 然后把它转换一下，存入结构体中去
                snprintf(stzhobtmind.t, 10, "%d", (int)(atof(tmp)*10));
            }
            GetXMLBuffer(strbuffer, "p", tmp, 10);      // xml解析出来的内容放到临时变量tmp中
            if(strlen(tmp) > 0)     // 如果取出来的临时变量不为空（表示有数据）
            {
                // 然后把它转换一下，存入结构体中去
                snprintf(stzhobtmind.p, 10, "%d", (int)(atof(tmp)*10));
            }
            GetXMLBuffer(strbuffer, "u", stzhobtmind.u, 10);
            GetXMLBuffer(strbuffer, "wd", stzhobtmind.wd, 10);
            GetXMLBuffer(strbuffer, "wf", tmp, 10); if(strlen(tmp) > 0) snprintf(stzhobtmind.wf, 10, "%d", (int)(atof(tmp)*10));
            GetXMLBuffer(strbuffer, "r", tmp, 10); if(strlen(tmp) > 0) snprintf(stzhobtmind.r, 10, "%d", (int)(atof(tmp)*10));
            GetXMLBuffer(strbuffer, "vis", tmp, 10); if(strlen(tmp) > 0) snprintf(stzhobtmind.vis, 10, "%d", (int)(atof(tmp)*10));

            // 把结构体中的数据插入表中
            if(stmt.execute() != 0)
            {
                // 失败的情况有哪些？是否全部的失败都需要写日志？
                // 失败的原因主要有二，1是记录重复，2是数据内容非法
                // 如果失败了怎么办？程序是否需要继续？是否rollback？是否返回false?
                // 如果失败的原因是内容非法，记录非法内容日志后继续，如果记录重复，不必记录日志，并且程序继续往下走
                if(stmt.m_cda.rc != 1062)
                {
                    // 不是记录重复导致的错误
                    logfile.Write("Buffer = %s\n", strbuffer);
                    logfile.Write("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message);
                }
            }
            else
            {
                insertcount++;
            }
        }

        // 删除文件，提交事务
        // File.CloseAndRemove();
        conn.commit();

        logfile.Write("已处理文件数%s(totalcount = %d, insertcount=%d), 耗时%.2f秒\n", Dir.m_FullFileName, totalcount, insertcount, Timer.Elapsed());

    }
    
    return true;
}