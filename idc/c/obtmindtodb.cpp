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
};

void EXIT(int sig);

// 业务处理主函数
bool _obtmindtodb(const char* pathname, char* connstr, char* charset);

class CZHOBTMIND
{
public:
    connection  *m_conn;        // 数据库连接
    CLogFile *m_logfile;        // 日志

    sqlstatement m_stmt;        // 插入表操作的sql

    char m_buffer[1024];             // 从文件中读取到的一行
    
    struct st_zhobtmind m_zhobtmind;    // 全国站点分钟观测数据结构
    
public:
    CZHOBTMIND();
    ~CZHOBTMIND();

    CZHOBTMIND(connection *conn, CLogFile *logfile);

    void BindConnLog(connection *conn, CLogFile *logfile);      // 把connection和CLogFile的地址传递进去
    bool SplitBuffer(char *strline);            // 把从哪文件中读取到的一行数据拆分到m_zhobtmind结构体中
    bool InsertTable();                         // 把m_zhobtmind结构体中的数据插入到T_ZHOBTMIND表中
};

CZHOBTMIND::CZHOBTMIND()
{
    m_conn = nullptr;
    m_logfile = nullptr;
}

CZHOBTMIND::CZHOBTMIND(connection *conn, CLogFile *logfile)
{
    m_conn = conn;
    m_logfile = logfile;
}

CZHOBTMIND::~CZHOBTMIND()
{
}

void CZHOBTMIND::BindConnLog(connection *conn, CLogFile *logfile)
{
    m_conn = conn;
    m_logfile = logfile;
}

// 把从哪文件中读取到的一行数据拆分到m_zhobtmind结构体中
bool CZHOBTMIND::SplitBuffer(char *strline)
{
    // 初始化结构体
    memset(&m_zhobtmind, 0, sizeof(struct st_zhobtmind));
    GetXMLBuffer(strline, "obtid", m_zhobtmind.obtid, 10);
    GetXMLBuffer(strline, "ddatetime", m_zhobtmind.ddatetime, 14);
    char tmp[11];       // 定义一个临时变量
    GetXMLBuffer(strline, "t", tmp, 10);      // xml解析出来的内容放到临时变量tmp中
    if(strlen(tmp) > 0)     // 如果取出来的临时变量不为空（表示有数据）
    {
        // 然后把它转换一下，存入结构体中去
        snprintf(m_zhobtmind.t, 10, "%d", (int)(atof(tmp)*10));
    }
    GetXMLBuffer(strline, "p", tmp, 10);      // xml解析出来的内容放到临时变量tmp中
    if(strlen(tmp) > 0)     // 如果取出来的临时变量不为空（表示有数据）
    {
        // 然后把它转换一下，存入结构体中去
        snprintf(m_zhobtmind.p, 10, "%d", (int)(atof(tmp)*10));
    }
    GetXMLBuffer(strline, "u", m_zhobtmind.u, 10);
    GetXMLBuffer(strline, "wd", m_zhobtmind.wd, 10);
    GetXMLBuffer(strline, "wf", tmp, 10); if(strlen(tmp) > 0) snprintf(m_zhobtmind.wf, 10, "%d", (int)(atof(tmp)*10));
    GetXMLBuffer(strline, "r", tmp, 10); if(strlen(tmp) > 0) snprintf(m_zhobtmind.r, 10, "%d", (int)(atof(tmp)*10));
    GetXMLBuffer(strline, "vis", tmp, 10); if(strlen(tmp) > 0) snprintf(m_zhobtmind.vis, 10, "%d", (int)(atof(tmp)*10));

    STRCPY(m_buffer, sizeof(m_buffer), strline);

    return true;
}

// 把m_zhobtmind结构体中的数据插入到T_ZHOBTMIND表中
bool CZHOBTMIND::InsertTable()
{
    // 只有当stmt对象没有绑定数据库对象的时候，才进行绑定(与数据库连接的绑定状态，0-未绑定，1-已绑定)
    if(m_stmt.m_state == 0)
    {
        m_stmt.connect(m_conn);
        m_stmt.prepare("insert into T_ZHOBTMIND(obtid, ddatetime, t, p, u, wd, wf, r, vis)\
        values(:1, str_to_date(:2, '%%Y%%m%%d%%H%%i%%s'), :3, :4, :5, :6, :7, :8, :9)");
        
        // 绑定参数
        m_stmt.bindin(1, m_zhobtmind.obtid, 10);
        m_stmt.bindin(2, m_zhobtmind.ddatetime, 14);
        m_stmt.bindin(3, m_zhobtmind.t,10);
        m_stmt.bindin(4, m_zhobtmind.p,10);
        m_stmt.bindin(5, m_zhobtmind.u,10);
        m_stmt.bindin(6, m_zhobtmind.wd,10);
        m_stmt.bindin(7, m_zhobtmind.wf,10);
        m_stmt.bindin(8, m_zhobtmind.r,10);
        m_stmt.bindin(9, m_zhobtmind.vis,10);
    }

    // 把结构体中的数据插入表中
    if(m_stmt.execute() != 0)
    {
        // 失败的情况有哪些？是否全部的失败都需要写日志？
        // 失败的原因主要有二，1是记录重复，2是数据内容非法
        // 如果失败了怎么办？程序是否需要继续？是否rollback？是否返回false?
        // 如果失败的原因是内容非法，记录非法内容日志后继续，如果记录重复，不必记录日志，并且程序继续往下走
        if(m_stmt.m_cda.rc != 1062)
        {
            // 不是记录重复导致的错误
            m_logfile->Write("Buffer = %s\n", m_buffer);
            m_logfile->Write("m_stmt.execute() failed.\n%s\n%s\n", m_stmt.m_sql, m_stmt.m_cda.message);
        }
        return false;
    }
    return true;
}

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
    CDir Dir;
    // 打开目录
    if(Dir.OpenDir(pathname, "*.xml") == false)
    {
        logfile.Write("Dir.OpenDir(%s, \"*.xml\") failed\n", pathname);
        return false;
    }

    CFile File;

    CZHOBTMIND zhobtmindObj(&conn, &logfile);

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
            // 读取文件的一行到strbuffer中，其中以 “</endl>”为字符串结束标志（为行读取结束标志）
            if(File.FFGETS(strbuffer, 1000, "<endl/>") == false) break;

            totalcount++;

            zhobtmindObj.SplitBuffer(strbuffer);

            if(zhobtmindObj.InsertTable() == true) insertcount++;

        }

        // 删除文件，提交事务
        // File.CloseAndRemove();
        conn.commit();

        logfile.Write("已处理文件数%s(totalcount = %d, insertcount=%d), 耗时%.2f秒\n", Dir.m_FullFileName, totalcount, insertcount, Timer.Elapsed());

    }
    
    return true;
}