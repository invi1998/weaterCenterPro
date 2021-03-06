// obtcodetodb.cpp 本程序用于将全国站点参数数据保存到数据库 T_ZHOBTCODE 表中

#include "_public.h"
#include "_mysql.h"

// 全国气象站点参数结构体。
struct st_stcode
{
  char provname[31]; // 省
  char obtid[11];    // 站号
  char cityname[31];  // 站名
  char lat[11];      // 纬度
  char lon[11];      // 经度
  char height[11];   // 海拔高度
};

vector<struct st_stcode> vstcode; // 存放全国气象站点参数的容器。

// 把站点参数文件中加载到vstcode容器中。
bool LoadSTCode(const char *inifile);

CLogFile logfile;

connection conn;            // 数据库连接对象

CPActive PActive;         // 进程心跳

void EXIT(int sig);

int main(int argc, char * argv[])
{
    // 帮助文档
    if (argc!=5)
    {
        printf("\n");
        printf("Using:./obtcodetodb inifile connstr charset logfile\n");

        printf("Example:/project/tools/bin/procctl 120 /project/idc/bin/obtcodetodb /project/idc/ini/stcode.ini \"192.168.31.133,root,password,mysql,3306\" utf8 /log/idc/obtcodetodb.log\n\n");

        printf("本程序用于把全国站点参数数据保存到数据库表中，如果站点不存在则插入，站点已存在则更新。\n");
        printf("inifile 站点参数文件名（全路径）。\n");
        printf("connstr 数据库连接参数：ip,username,password,dbname,port\n");
        printf("charset 数据库的字符集。\n");
        printf("logfile 本程序运行的日志文件名。\n");
        printf("程序每120秒运行一次，由procctl调度。\n\n\n");

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

    // 把全国站点参数文件加载到vstcode容器中
    if(LoadSTCode(argv[1]) == false)
    {
        printf("打开日志文件失败（%s）。\n",argv[4]);
        return -1;
    }

    logfile.Write("加载参数文件（%s）成功, 站点数（%d)\n", argv[1], vstcode.size());

    // 连接数据库
    if(conn.connecttodb(argv[2], argv[3]) != 0)
    {
        logfile.Write("数据库【%s】连接失败\n失败原因：%s\n", argv[2], conn.m_cda.message);
        return -1;
    }

    logfile.Write("数据库连接成功！\n");

    // 表结构如下
    // +----------+-------------+------+-----+-------------------+-----------------------------+
    // | Field    | Type        | Null | Key | Default           | Extra                       |
    // +----------+-------------+------+-----+-------------------+-----------------------------+
    // | obtid    | varchar(10) | NO   | PRI | NULL              |                             |
    // | cityname | varchar(30) | NO   |     | NULL              |                             |
    // | provname | varchar(30) | NO   |     | NULL              |                             |
    // | lat      | int(11)     | NO   |     | NULL              |                             |
    // | lon      | int(11)     | NO   |     | NULL              |                             |
    // | height   | int(11)     | YES  |     | NULL              |                             |
    // | upttime  | timestamp   | NO   |     | CURRENT_TIMESTAMP | on update CURRENT_TIMESTAMP |
    // | keyid    | int(11)     | NO   | UNI | NULL              | auto_increment              |
    // +----------+-------------+------+-----+-------------------+-----------------------------+

    struct st_stcode stcode;

    // 准备插入表的sql语句
    sqlstatement stmtins(&conn);
    // 注意：这里因为我们是使用整数来表示浮点数，所以要注意单位转换（经度纬度 这两个字段都需要*100，海拔高度需要*10）
    stmtins.prepare("insert into T_ZHOBTCODE(obtid, cityname, provname, lat, lon, height, upttime) values(:1, :2, :3, :4*100, :5*100, :6*10, now())");
    // 绑定输入变量地址
    stmtins.bindin(1, stcode.obtid, 10);
    stmtins.bindin(2, stcode.cityname, 30);
    stmtins.bindin(3, stcode.provname, 30);
    stmtins.bindin(4, stcode.lat, 10);
    stmtins.bindin(5, stcode.lon, 10);
    stmtins.bindin(6, stcode.height, 10);

    // 准备更新表的sql语句
    sqlstatement stmtupt(&conn);
    // 注意：这里因为我们是使用整数来表示浮点数，所以要注意单位转换（经度纬度 这两个字段都需要*100，海拔高度需要*10）
    stmtupt.prepare("update T_ZHOBTCODE set cityname=:1, provname=:2, lat=:3*100, lon=:4*100, height=:5*10, upttime=now() where obtid=:6");
    // 遍历vstcode容器
    // 绑定输入变量地址
    stmtupt.bindin(1, stcode.cityname, 30);
    stmtupt.bindin(2, stcode.provname, 30);
    stmtupt.bindin(3, stcode.lat, 10);
    stmtupt.bindin(4, stcode.lon, 10);
    stmtupt.bindin(5, stcode.height, 10);
    stmtupt.bindin(6, stcode.obtid, 10);

    // 插入记录数、更新记录数（初始化为0）
    int inscount = 0, uptcount = 0;
    CTimer Timer;

    for(auto iter = vstcode.begin(); iter != vstcode.end(); ++iter)
    {
        // 从容器中取出一条记录到结构体stcode中
        memcpy(&stcode, &(*iter), sizeof(struct st_stcode));

        // 执行插入的sql语句
        if(stmtins.execute() != 0)
        {
            // 如果记录已经存在，就执行更新的sql语句(这里如何判断记录是否已经存在，如果记录存在，sql执行的返回结果是1062)
            if(stmtins.m_cda.rc == 1062)
            {
                // 执行更新操作
                if(stmtupt.execute() != 0)
                {
                    logfile.Write("stmtupt.execute() failed \n%s\n", stmtupt.m_cda.message);
                    return -1;
                }
                else
                {
                    // 如果更新成功，把更新的记录数+1
                    uptcount++;
                }
            }
            else
            {
                logfile.Write("stmtins.execute() failed \n%s\n", stmtins.m_cda.message);
                return -1;
            }
        }
        else
        {
            // 如果插入成功，把插入的记录数+1
            inscount++;
        }
    }

    // 把总的记录数，插入记录数，更新记录数，消耗耗时 记录日志
    logfile.Write("总的记录数=%d，插入记录数=%d，更新记录数=%d，消耗耗时%.2f \n", vstcode.size(), inscount, uptcount, Timer.Elapsed());

    // 提交事务
    conn.commit();

    return 0;
}

// 把站点参数文件中加载到vstcode容器中。
bool LoadSTCode(const char *inifile)
{
    CFile File;

    // 打开站点参数文件。
    if (File.Open(inifile,"r")==false)
    {
        logfile.Write("File.Open(%s) failed.\n",inifile); return false;
    }

    char strBuffer[301];

    CCmdStr CmdStr;

    struct st_stcode stcode;

    while (true)
    {
        // 从站点参数文件中读取一行，如果已读取完，跳出循环。
        if (File.Fgets(strBuffer,300,true)==false) break;

        // 把读取到的一行拆分。
        CmdStr.SplitToCmd(strBuffer,",",true);

        if (CmdStr.CmdCount()!=6) continue;     // 扔掉无效的行。

        // 把站点参数的每个数据项保存到站点参数结构体中。
        memset(&stcode,0,sizeof(struct st_stcode));
        CmdStr.GetValue(0, stcode.provname,30); // 省
        CmdStr.GetValue(1, stcode.obtid,10);    // 站号
        CmdStr.GetValue(2, stcode.cityname,30);  // 站名
        CmdStr.GetValue(3, stcode.lat,10);      // 纬度
        CmdStr.GetValue(4, stcode.lon,10);      // 经度
        CmdStr.GetValue(5, stcode.height,10);   // 海拔高度

        // 把站点参数结构体放入站点参数容器。
        vstcode.push_back(stcode);
    }

    /*
    for (int ii=0;ii<vstcode.size();ii++)
        logfile.Write("provname=%s,obtid=%s,cityname=%s,lat=%.2f,lon=%.2f,height=%.2f\n",\
                    vstcode[ii].provname,vstcode[ii].obtid,vstcode[ii].cityname,vstcode[ii].lat,\
                    vstcode[ii].lon,vstcode[ii].height);
    */

    return true;
}

void EXIT(int sig)
{
    logfile.Write("程序退出，sig=%d\n\n",sig);

    // 断开数据库连接
    conn.disconnect();

    exit(0);
}
