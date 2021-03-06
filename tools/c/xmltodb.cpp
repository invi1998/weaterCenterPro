// 程序名：xmltodb.cpp，本程序是数据中心的公共功能模块，用于把xml文件入库到MySQL的表中。

#include "_public.h"
#include "_mysql.h"
#include "_tools.h"

#define MAXCOLCOUNT 500             // 每个表字段的最大值
    
char *strcolvalue[MAXCOLCOUNT];       // 存放xml每一行中解析出来的值

struct st_arg
{
  char connstr[101];     // 数据库的连接参数。
  char charset[51];      // 数据库的字符集。
  char inifilename[301]; // 数据入库的参数配置文件。
  char xmlpath[301];     // 待入库xml文件存放的目录。
  char xmlpathbak[301];  // xml文件入库后的备份目录。
  char xmlpatherr[301];  // 入库失败的xml文件存放的目录。
  int  timetvl;          // 本程序运行的时间间隔，本程序常驻内存。
  int  timeout;          // 本程序运行时的超时时间。
  char pname[51];        // 本程序运行时的程序名。
} starg;

CPActive PActive;

CLogFile logfile;

connection conn;

CTABCOLS tabcols;       // 数据字典类对象，获取表全部的字段和主键字段

char strinsertsql[10241];       // 插入表的sql语句
char strupdatesql[10241];       // 更新表的sql语句

sqlstatement stmtins, stmtupt;      // 插入和更新两个sqlstatement对象

int totalcount,inscount,uptcount;    // xml文件的总记录数、插入记录数和更新记录数。

struct st_xmltotable
{
    char filename[101];         // xml文件的匹配规则，用逗号分割
    char tname[31];             // 待入库的表名
    int uptbz;                  // 更新标志，1-更新， 2-不更新
    char execsql[301];          // 处理xml文件之前，执行的sql语句
} stxmltotable;

vector<struct st_xmltotable> vxmltotable;           // 数据入库的参数容器

// 把数据入库的参数配置文件starg.inifilename加载到vxmltotable容器中
bool loadxmltotable();

// 从vxmltotable容器中查找xmlfilename的入库参数的函数，将查找结果存放在stxmltotable结构体中
bool findxmltotable(char *xmlfilename);

// 显示程序的帮助
void _help(char *argv[]);

// 把xml解析到参数starg结构中
bool _xmltoarg(char *strxmlbuffer);

// 有了表的字段和主键信息，就可以拼接生成插入和更新表数据的sql
void crtsql();
 
void EXIT(int sig);

// 业务处理主函数
bool _xmltodb();

// xml文件入库主函数，返回值 0 - 成功，其他都是失败，失败的情况有很多
int _xmltodbFile(char *fullfilename, char *filename);

// 把xml文件（fullFileName）从srcpath移动到dstpath参数指定的目录中(一般文件移动不会出现问题，如果出现了问题，那么大多都是权限或者磁盘空间满了)
bool xmltobakerr(char* fullFileName, char *srcpath, char *dstpath);

// prepare插入和更新的sql语句，绑定输入变量
void preparesql();

// 在处理xml文件之前，如果stxmltotable.execsql不为空，就执行它
bool execsqlpre();

// 解析xml，存放在已经绑定的输入变量strcolvalue中
void splitbuffer(char *strbuffer);

int main(int argc,char *argv[])
{
    if (argc!=3) { _help(argv); return -1; }

    // 关闭全部的信号和输入输出。
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
    // 但请不要用 "kill -9 +进程号" 强行终止。
    CloseIOAndSignal();
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    if (logfile.Open(argv[1],"a+")==false)
    {
        printf("打开日志文件失败（%s）。\n",argv[1]); return -1;
    }

    // 把xml解析到参数starg结构中
    if (_xmltoarg(argv[2])==false) return -1;

    PActive.AddPInfo(starg.timeout, starg.pname);

    // 业务处理主函数
    _xmltodb();

}

// 显示程序的帮助
void _help(char *argv[])
{
    printf("Using:/project/tools/bin/xmltodb logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 10 /project/tools/bin/xmltodb /log/idc/xmltodb_vip1.log \"<connstr>127.0.0.1,root,password,mysql,3306</connstr><charset>utf8</charset><inifilename>/project/tools/ini/xmltodb.xml</inifilename><xmlpath>/idcdata/xmltodb/vip1</xmlpath><xmlpathbak>/idcdata/xmltodb/vip1bak</xmlpathbak><xmlpatherr>/idcdata/xmltodb/vip1err</xmlpatherr><timetvl>5</timetvl><timeout>50</timeout><pname>xmltodb_vip1</pname>\"\n\n");

    printf("本程序是数据中心的公共功能模块，用于把xml文件入库到MySQL的表中。\n");
    printf("logfilename   本程序运行的日志文件。\n");
    printf("xmlbuffer     本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据库的连接参数，格式：ip,username,password,dbname,port。\n");
    printf("charset     数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
    printf("inifilename 数据入库的参数配置文件。\n");
    printf("xmlpath     待入库xml文件存放的目录。\n");
    printf("xmlpathbak  xml文件入库后的备份目录。\n");
    printf("xmlpatherr  入库失败的xml文件存放的目录。\n");
    printf("timetvl     本程序的时间间隔，单位：秒，视业务需求而定，2-30之间。\n");
    printf("timeout     本程序的超时时间，单位：秒，视xml文件大小而定，建议设置30以上。\n");
    printf("pname       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}

// 把xml解析到参数starg结构中
bool _xmltoarg(char *strxmlbuffer)
{
    memset(&starg,0,sizeof(struct st_arg));

    GetXMLBuffer(strxmlbuffer,"connstr",starg.connstr,100);
    if (strlen(starg.connstr)==0) { logfile.Write("connstr is null.\n"); return false; }

    GetXMLBuffer(strxmlbuffer,"charset",starg.charset,50);
    if (strlen(starg.charset)==0) { logfile.Write("charset is null.\n"); return false; }

    GetXMLBuffer(strxmlbuffer,"inifilename",starg.inifilename,300);
    if (strlen(starg.inifilename)==0) { logfile.Write("inifilename is null.\n"); return false; }

    GetXMLBuffer(strxmlbuffer,"xmlpath",starg.xmlpath,300);
    if (strlen(starg.xmlpath)==0) { logfile.Write("xmlpath is null.\n"); return false; }

    GetXMLBuffer(strxmlbuffer,"xmlpathbak",starg.xmlpathbak,300);
    if (strlen(starg.xmlpathbak)==0) { logfile.Write("xmlpathbak is null.\n"); return false; }

    GetXMLBuffer(strxmlbuffer,"xmlpatherr",starg.xmlpatherr,300);
    if (strlen(starg.xmlpatherr)==0) { logfile.Write("xmlpatherr is null.\n"); return false; }

    GetXMLBuffer(strxmlbuffer,"timetvl",&starg.timetvl);
    if (starg.timetvl< 2) starg.timetvl=2;   
    if (starg.timetvl>30) starg.timetvl=30;

    GetXMLBuffer(strxmlbuffer,"timeout",&starg.timeout);
    if (starg.timeout==0) { logfile.Write("timeout is null.\n"); return false; }

    GetXMLBuffer(strxmlbuffer,"pname",starg.pname,50);
    if (strlen(starg.pname)==0) { logfile.Write("pname is null.\n"); return false; }

    return true;
}

void EXIT(int sig)
{
    logfile.Write("程序退出，sig=%d\n\n",sig);

    conn.disconnect();

    exit(0);
}

// 业务处理主函数
bool _xmltodb()
{

    // 把数据入库的参数配置文件starg.inifilename加载到容器中

    // 加载入库参数的计数器，初始化为50是为了在第一次进入循环的时候就加载参数
    int counter = 50;

    CDir Dir;

    while (true)
    {
        if(counter++ > 30)
        {
            counter = 0;    // 从新计数
            // 把数据入库的参数配置文件starg.inifilename加载到vxmltotable容器中
            if(loadxmltotable() == false) return false;
        }

        // 打开starg.xmlpath目录(不打开子目录，需要排序，最多一次打开10000个文件)
        if(Dir.OpenDir(starg.xmlpath, "*.XML", 10000, false, true) == false)
        {
            logfile.Write("Dir.OpenDir(%s, \"*.XML\", 10000, false, true) failed\n", starg.xmlpath);
            return false;
        }

        
        // 遍历目录中的文件名
        while(true)
        {
            // 读取目录，得到一个xml文件
            if(Dir.ReadDir() == false) break;

            if(conn.m_state == 0)
            {
                if (conn.connecttodb(starg.connstr,starg.charset) != 0)
                {
                    printf("connect database(%s) failed.\n%s\n",starg.connstr,conn.m_cda.message); return false;
                }

                logfile.Write("connect database(%s) ok.\n",starg.connstr);
            }

            logfile.Write("处理文件%s...", Dir.m_FullFileName);

            // 处理xml文件
            // 调用文件处理子函数
            int iret = _xmltodbFile(Dir.m_FullFileName, Dir.m_FileName);

            PActive.UptATime();

            // 处理xml文件成功，写日志，备份文件
            if(iret == 0)
            {
                logfile.WriteEx(" ok(%s,total=%d,insert=%d,update=%d).\n",stxmltotable.tname,totalcount,inscount,uptcount);

                // 把xml文件移动到starg.xmlpathbak参数指定的目录中(一般文件移动不会出现问题，如果出现了问题，那么大多都是权限或者磁盘空间满了)
                if(xmltobakerr(Dir.m_FullFileName, starg.xmlpath, starg.xmlpathbak) == false) return false;
            }

            // 处理文件失败。分情况讨论
            if(iret == 1)       // iret == 1，找不到入库参数
            {
                logfile.WriteEx("failed. [没有配置文件入库参数]\n");
                // 把xml文件移动到starg.xmlpatherr参数指定的目录中(一般文件移动不会出现问题，如果出现了问题，那么大多都是权限或者磁盘空间满了)
                if(xmltobakerr(Dir.m_FullFileName, starg.xmlpath, starg.xmlpatherr) == false) return false;
            }

            // 数据库错误，函数返回，程序将退出
            if(iret == 4)
            {
                logfile.WriteEx("   failed. 数据库错误\n");
                return false;
            }

            // 如果是表不存在
            if(iret == 2)
            {
                logfile.WriteEx("failed. [待入库的表%s不存在]\n", stxmltotable.tname);
                // 把xml文件移动到starg.xmlpatherr参数指定的目录中(一般文件移动不会出现问题，如果出现了问题，那么大多都是权限或者磁盘空间满了)
                if(xmltobakerr(Dir.m_FullFileName, starg.xmlpath, starg.xmlpatherr) == false) return false;
            }

            // 在处理xml文件之前，如果执行stxmltotable.execsql失败，函数返回6，那么程序将退出
            if(iret == 6)
            {
                logfile.WriteEx("failed. [执行execsql失败]\n");
                return false;
            }

            // 打开xml文件失败
            if(iret == 3)
            {
                logfile.WriteEx("failed. [打开xml文件失败]\n");
                return false;
            }

            // 数据表字段大于程序承载的最大字段数
            if(iret == 5)
            {
                return false;
            }
        }

        // 如果刚才这次扫描有文件，表示不空闲，可能不断的有文件生成，就不sleep了
        if(Dir.m_vFileName.size() == 0) sleep(starg.timetvl);

        PActive.UptATime();
    }

    return true;
}

// 把数据入库的参数配置文件starg.inifilename加载到vxmltotable容器中
bool loadxmltotable()
{
    // 清空容器
    vxmltotable.clear();

    CFile  File;

    if(File.Open(starg.inifilename, "r") == false)
    {
        logfile.Write("File.Open(%s, \"r\") failed \n", starg.inifilename);
        return false;
    }

    char strBuffer[501];
    
    while (true)
    {
        if(File.FFGETS(strBuffer, 500, "<endl/>") == false) break;

        memset(&stxmltotable, 0, sizeof(struct st_xmltotable));

        GetXMLBuffer(strBuffer, "filename", stxmltotable.filename, 100);    // xml文件的匹配规则，用逗号分割
        GetXMLBuffer(strBuffer, "tname", stxmltotable.tname, 30);           // 待入库的表名
        GetXMLBuffer(strBuffer, "uptbz", &stxmltotable.uptbz);              // 更新标志：1-更新，2-不更新
        GetXMLBuffer(strBuffer, "execsql", stxmltotable.execsql, 300);      // 处理xml文件之前，执行的sql语句

        vxmltotable.push_back(stxmltotable);
    }

    logfile.Write("loadxmltotable(%s) ok\n", starg.inifilename);

    return true;
}

// 从vxmltotable容器中查找xmlfilename的入库参数的函数，将查找结果存放在stxmltotable结构体中
bool findxmltotable(char *xmlfilename)
{
    for (int ii=0;ii<vxmltotable.size();ii++)
    {
        if (MatchStr(xmlfilename,vxmltotable[ii].filename)==true)
        {
            memcpy(&stxmltotable, &vxmltotable[ii], sizeof(struct st_xmltotable));
            return true;
        }
    }

    return false;
}

// xml文件入库主函数，返回值 0 - 成功，其他都是失败，失败的情况有很多
int _xmltodbFile(char *fullfilename, char *filename)
{
    totalcount=inscount=uptcount=0;
    // 先从vxmltotable容器中查找filename的入库参数，存放在stxmltotable容器中
    if(findxmltotable(filename) == false) return 1;

    // 释放上次处理xml文件时，为字段分配的内存
    for(int i = 0; i < tabcols.m_allcount; i++)
    {
        if(strcolvalue[i] != 0)
        {
            delete strcolvalue[i];
            strcolvalue[i] = 0;
        }
    }

    // 处理文件之前，先查询mysql的数据字典，把表的字段信息拿出来（获取表全部的字段和主键信息）

    // 获取表全部的字段和主键信息，如果获取失败，应该是数据库连接已经失效
    // 在本程序的运行过程中，如果数据库出现异常，一定会在这里被发现
    if(tabcols.allcols(&conn, stxmltotable.tname) == false)
    {
        return 4;       // 数据库发生错误，放回4
    }

    if(tabcols.pkcols(&conn, stxmltotable.tname) == false)
    {
        return 4;       // 数据库发生错误，放回4
    }

    // 如果表 tabcols.m_allcount为0，表示表根本不存在，放回2
    if(tabcols.m_allcount == 0)
    {
        return 2;       // 待入库表不存在
    }

    if(tabcols.m_allcount > MAXCOLCOUNT)
    {
        logfile.Write("数据表字段（%d）大于程序承载的最大字段数（%d）\n", tabcols.m_allcount, MAXCOLCOUNT);
        return 5;
    }

    // 为每个字段分配内存
    for (int ii=0;ii<tabcols.m_allcount;ii++) strcolvalue[ii]=new char[tabcols.m_vallcols[ii].collen+1];

    // 有了表的字段和主键信息，就可以拼接生成插入和更新表数据的sql
    crtsql();

    // prepare插入和更新的sql语句，绑定输入变量
    preparesql();

    // 在处理xml文件之前，如果stxmltotable.execsql不为空，就执行它
    if(execsqlpre() == false) return 6;        // sql执行失败，返回6

    // 打开xml文件
    CFile File;

    if(File.Open(fullfilename, "r") == false)
    {
        // 如果打开文件失败，那么这里需要回滚事务，然后返回错误代码3
        conn.rollback();
        return 3;
    }

    char strbuffer[10241];

    while (true)
    {
        // 从xml文件中读取一行
        if(File.FFGETS(strbuffer, 10240, "<endl/>") == false) break;

        totalcount++;

        // 解析xml，存放在已经绑定的输入变量strcolvalue中
        splitbuffer(strbuffer);

        // 执行插入和更新的sql
        if(stmtins.execute() != 0)
        {
            // 执行失败，如果返回的错误码是 1062 ，表示违反了唯一性约束，表示记录已经存在，那么就执行更新的sql语句
            if(stmtins.m_cda.rc == 1062)
            {
                // 是否执行更新的sql，还需要判断入库员参数的更新标志
                if(stxmltotable.uptbz == 1)
                {
                    if(stmtupt.execute() != 0)
                    {
                        // 如果更新失败，记录日志，把出错的行和错误内容记录下来，然后函数不返回，继续处理数据，也就是说，不理这行
                        logfile.Write("%s   ", strbuffer);
                        logfile.WriteEx("stmtupt.execute() failed\n%s\n%s\n", stmtupt.m_sql, stmtupt.m_cda.message);

                        // 判断错误代码，如果是数据库连接已经失效，无法继续，只能返回
                        // 1053 - 在操作过程中，服务器关闭
                        // 2013 - 查询过程中丢失了与mysql服务器的连接
                        if(stmtupt.m_cda.rc == 1053 || stmtupt.m_cda.rc == 2013) return 4;
                    }
                    else
                    {
                        uptcount++;
                    }
                }
            }
            else
            {
                // 如果插入失败，记录日志，把出错的行和错误内容记录下来，然后函数不返回，继续处理数据，也就是说，不理这行
                logfile.Write("%s   ", strbuffer);
                logfile.WriteEx("stmtins.execute() failed\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
                // 判断错误代码，如果是数据库连接已经失效，无法继续，只能返回
                // 1053 - 在操作过程中，服务器关闭
                // 2013 - 查询过程中丢失了与mysql服务器的连接
                if(stmtins.m_cda.rc == 1053 || stmtins.m_cda.rc == 2013) return 4;
            }
        }
        else
        {
            inscount++;
        }
    }

    conn.commit();
    
    return 0;
}

// 把xml文件（fullFileName）从srcpath移动到dstpath参数指定的目录中(一般文件移动不会出现问题，如果出现了问题，那么大多都是权限或者磁盘空间满了)
bool xmltobakerr(char* fullFileName, char *srcpath, char *dstpath)
{
    // 生成一个临时变量，用于保存目标文件名
    char dstfilename[301];

    STRCPY(dstfilename, sizeof(dstfilename), fullFileName);

    // 这里注意第四个参数，这里填false
    // bloop：是否循环执行替换。
    // 注意：
    // 1、如果str2比str1要长，替换后str会变长，所以必须保证str有足够的空间，否则内存会溢出。
    // 2、如果str2中包含了str1的内容，且bloop为true，这种做法存在逻辑错误，UpdateStr将什么也不做
    UpdateStr(dstfilename, srcpath, dstpath, false);

    // 然后移动文件
    if(RENAME(fullFileName, dstfilename) == false)
    {
        logfile.Write("RENAME(%s, %s) failed\n", fullFileName, dstfilename);
        return false;
    }

    return true;
}

// 有了表的字段和主键信息，就可以拼接生成插入和更新表数据的sql
void crtsql()
{
    memset(strinsertsql, 0, sizeof(strinsertsql));
    memset(strupdatesql, 0, sizeof(strupdatesql));

    // 生成插入表的sql语句 insert into 表名（%s） values(%s)
    char strinsertp1[3001];     // insert语句的字段列表
    char strinsertp2[3001];     // insert语句的values后面的内容

    memset(strinsertp1, 0, sizeof(strinsertp1));
    memset(strinsertp2, 0, sizeof(strinsertp2));

    int colseq = 1;         // values部分字段的序号

    for(auto iter = tabcols.m_vallcols.begin(); iter != tabcols.m_vallcols.end(); ++iter)
    {
        // upttime和keyid这两个字段不需要处理
        if(strcmp((*iter).colname, "upttime") == 0 || strcmp((*iter).colname, "keyid") == 0) continue;

        // 拼接 strinsertp1
        strcat(strinsertp1, (*iter).colname);
        strcat(strinsertp1, ",");

        // 拼接strinserttp2，需要区分date字段和非date字段
        char strtemp[101];
        if(strcmp((*iter).datatype, "date") != 0)
        {
            SNPRINTF(strtemp, 100, sizeof(strtemp), ":%d", colseq);
        }
        else
        {
            SNPRINTF(strtemp, 100, sizeof(strtemp), "str_to_date(:%d, '%%%%Y%%%%m%%%%d%%%%H%%%%i%%%%s')", colseq);
        }

        strcat(strinsertp2, strtemp);

        strcat(strinsertp2, ",");
        
        colseq++;
    }

    // 删除拼接过程中多余的逗号
    strinsertp1[strlen(strinsertp1) - 1] = 0;
    strinsertp2[strlen(strinsertp2) - 1] = 0;

    SNPRINTF(strinsertsql, 10240, sizeof(strinsertsql), "insert into %s(%s) values(%s)", stxmltotable.tname, strinsertp1, strinsertp2);

    if(stxmltotable.uptbz != 1) return;     // 如果更新标志不是 1（更新），就返回，不用生成修改表的sql语句了

    // 生成修改表的sql语句
    // update T_ZHOBTMIND1 set t=:1,p=:2,u=:3,wd=:4,wf=:5,r=:6,vis=:7,upttime=now(),mint=:8,minttime=str_to_date(:9, '%Y%m%d%H%i%s') where obtid=:10 and ddatetime=str_to_date(:11, '%Y%m%d%H%i%s')

    // 更新tabcols.m_vallcols中的pkseq字段，在拼接update语句的时候会用到它
    for(auto iter = tabcols.m_vpkcols.begin(); iter != tabcols.m_vpkcols.end(); ++iter)
    {
        for(auto ite = tabcols.m_vallcols.begin(); ite != tabcols.m_vallcols.end(); ++ite)
        {
            if(strcmp((*iter).colname, (*ite).colname) == 0)
            {
                // 更新m_vallcols容器中的pkseq
                (*ite).pkseq = (*iter).pkseq;
                break;
            }
        }
    }

    // 先拼接sql语句开始的部分
    sprintf(strupdatesql, "update %s set ", stxmltotable.tname);

    colseq = 1;

    // 拼接sql语句的set部分
    for(auto iter = tabcols.m_vallcols.begin(); iter != tabcols.m_vallcols.end(); ++iter)
    {
        // keyid字段不需要处理
        if(strcmp((*iter).colname, "keyid") == 0) continue;

        // 如果是主键字段，也不需要拼接在set后面
        if((*iter).pkseq != 0) continue;

        // 如果是更新时间字段，upttime字段直接等于now()，这么做是为了数据库的兼容性考虑
        if(strcmp((*iter).colname, "upttime") == 0)
        {
            strcat(strupdatesql, "upttime=now(),");
            continue;
        }

        // 其他字段要区分date字段和非date字段
        char strtemp[101];

        if(strcmp((*iter).datatype, "date") != 0)
        {
            SNPRINTF(strtemp, 100, sizeof(strtemp), "%s=:%d", (*iter).colname, colseq);
        }
        else
        {
            SNPRINTF(strtemp, 100, sizeof(strtemp), "%s=str_to_date(:%d, '%%%%Y%%%%m%%%%d%%%%H%%%%i%%%%s')", (*iter).colname, colseq);
        }

        strcat(strupdatesql, strtemp);
        strcat(strupdatesql, ",");
        colseq++;
    }

    strupdatesql[strlen(strupdatesql) - 1] = 0;

    // 然后拼接update语句where后面的部分
    strcat(strupdatesql, " where 1=1 ");        // 用 1=1 是为了后面的拼接方便，这是常用的处理方法

    for(auto iter = tabcols.m_vallcols.begin(); iter != tabcols.m_vallcols.end(); ++iter)
    {
        if((*iter).pkseq == 0) continue;        // 如果不是主键字段，跳过

        // 把主键字段拼接到update语句中。注意需要区分date和非date字段
        char strtemp[101];

        if(strcmp((*iter).datatype, "date") != 0)
        {
            SNPRINTF(strtemp, 100, sizeof(strtemp), " and %s=:%d", (*iter).colname, colseq);
        }
        else
        {
            SNPRINTF(strtemp, 100, sizeof(strtemp), " and %s=str_to_date(:%d, '%%%%Y%%%%m%%%%d%%%%H%%%%i%%%%s')", (*iter).colname, colseq);
        }

        strcat(strupdatesql, strtemp);
        colseq++;

    }

}

// prepare插入和更新的sql语句，绑定输入变量
void preparesql()
{
    // 绑定插入sql语句的输入变量
    stmtins.connect(&conn);
    stmtins.prepare(strinsertsql);

    int colseq = 1;     // values部分的序号
    int indexValue = -1;

    for(auto iter = tabcols.m_vallcols.begin(); iter != tabcols.m_vallcols.end(); ++iter)
    {
        indexValue++;
        // upttime和keyid这两个字段不需要处理
        if(strcmp((*iter).colname, "upttime") == 0 || strcmp((*iter).colname, "keyid") == 0) continue;

        // 注意：strcolvalue数组的使用功能不是连续，是和tabcols.m_callcols的下标是一一对应的
        stmtins.bindin(colseq, strcolvalue[indexValue], (*iter).collen);
        colseq++;
    }

    // 绑定更新sql语句的输入变量
    // 如果入库参数中的更新标志位不更新，就不必处理update语句，函数返回
    if(stxmltotable.uptbz != 1) return;

    stmtupt.connect(&conn);
    stmtupt.prepare(strupdatesql);

    colseq = 1;     // set部分和where部分的序号
    indexValue = -1;

    // 绑定set部分的输入参数
    for(auto iter = tabcols.m_vallcols.begin(); iter != tabcols.m_vallcols.end(); ++iter)
    {
        indexValue++;
        // upttime和keyid这两个字段不需要处理
        if(strcmp((*iter).colname, "upttime") == 0 || strcmp((*iter).colname, "keyid") == 0) continue;

        // 如果是主键字段，就跳过它，不需要拼接在set后面
        if((*iter).pkseq != 0) continue;

        // 注意：strcolvalue数组的使用功能不是连续，是和tabcols.m_callcols的下标是一一对应的
        stmtupt.bindin(colseq, strcolvalue[indexValue], (*iter).collen);
        colseq++;
    }

    // 绑定where部分的输入参数

    indexValue = -1;

    // 绑定set部分的输入参数
    for(auto iter = tabcols.m_vallcols.begin(); iter != tabcols.m_vallcols.end(); ++iter)
    {
        indexValue++;

        // 如果是主键字段，就跳过它，不需要拼接在set后面,只有主键字段才拼接到where后面
        if((*iter).pkseq == 0) continue;

        // 注意：strcolvalue数组的使用功能不是连续，是和tabcols.m_callcols的下标是一一对应的
        stmtupt.bindin(colseq, strcolvalue[indexValue], (*iter).collen);
        colseq++;
    }

}

// 在处理xml文件之前，如果stxmltotable.execsql不为空，就执行它
bool execsqlpre()
{
    if(strlen(stxmltotable.execsql) == 0) return true;

    sqlstatement stmt;
    stmt.connect(&conn);
    stmt.prepare(stxmltotable.execsql);

    if(stmt.execute() != 0)
    {
        logfile.Write("stmt.execute() failed\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message);
        return false;
    }

    // 注意，这里不需要提交事务，执行这个sql语句 和 处理xml文件需要放在同一个事务中。
    // 如果这里提交了事务，假设处理xml文件失败，那么就可能会导致原来的表数据丢失

    return true;
}

// 解析xml，存放在已经绑定的输入变量strcolvalue中
void splitbuffer(char *strbuffer)
{
    // 初始化strcolvalue数组
    for(int i = 0; i < tabcols.m_allcount; i++)
    {
        memset(strcolvalue[i], 0, tabcols.m_vallcols[i].collen + 1);
    }
        
    char strtemp[31];
    int index = -1;
    for(auto iter = tabcols.m_vallcols.begin(); iter != tabcols.m_vallcols.end(); ++iter)
    {
        index++;
        // 如果是日期时间字段，提取数值就可以了
        // 也就是说xml文件中的日期时间只要包含了yyyymmddhh24miss就行，可以是任意分隔符
        if(strcmp((*iter).datatype, "date") == 0)
        {
            GetXMLBuffer(strbuffer, (*iter).colname, strtemp, (*iter).collen);

            // 从一个字符串中提取出数字、符号和小数点，存放到另一个字符串中。
            // src：原字符串。
            // dest：目标字符串。
            // bsigned：是否包括符号（+和-），true-包括；false-不包括。
            // bdot：是否包括小数点的圆点符号，true-包括；false-不包括。
            PickNumber(strtemp, strcolvalue[index], false, false);
            continue;
        }

        // 如果是数值字段，只提取数字，+ - 和圆点
        if(strcmp((*iter).datatype, "number") == 0)
        {
            GetXMLBuffer(strbuffer, (*iter).colname, strtemp, (*iter).collen);

            // 如果是字符字段，直接提取
            // 从一个字符串中提取出数字、符号和小数点，存放到另一个字符串中。
            // src：原字符串。
            // dest：目标字符串。
            // bsigned：是否包括符号（+和-），true-包括；false-不包括。
            // bdot：是否包括小数点的圆点符号，true-包括；false-不包括。
            PickNumber(strtemp, strcolvalue[index], true, true);
            continue;
        }

        // 如果是数值字段，只提取数字，+ - 和圆点
        GetXMLBuffer(strbuffer, (*iter).colname, strcolvalue[index], (*iter).collen);

    }
}
