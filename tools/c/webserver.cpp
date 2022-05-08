/*
 * 程序名：webserver.cpp，此程序是数据服务总线的服务端程序。

 * author：invi
*/
#include "_public.h"
#include "_ooci.h"

#define MAXCONNS 10                 // 数据库连接池的大小
connection conns[MAXCONNS];         // 数据库连接池数组
pthread_mutex_t mutex[MAXCONNS];    // 数据库连接池的锁

bool initconns();                   // 初始化数据库连接池
connection *getconns();             // 从数据库连接池中获取一个空闲的连接
bool freecons(connection* conn);    // 释放、归还 数据库连接
bool destroyconns();                // 释放数据库连接占用的资源（断开数据库连接 并 销毁锁）

CLogFile logfile;   // 服务程序运行日志对象
CTcpServer TcpServer;   // tcp服务端类对象

struct st_arg
{
    char connstr[101];      // 数据库连接参数
    char charset[51];       // 数据库连接的字符集
    int port;               // web服务监听端口
} starg;


void EXIT(int sig);   // 线程退出函数

void* thmain(void *arg);		// 线程入口函数

void thcleanup(void *arg);		// 线程清理函数

pthread_spinlock_t spin;		// 自旋锁（对线程id容器这个公共资源进行加锁）

// 存放线程id
vector<pthread_t> vpid;

// 读取客户端的报文
int ReadT(const int sockfd, char* buffer, const int size, const int itimeout);

// 显示程序的帮助
void _help(char *argv[]);

// 把xml解析到参数starg结构中
bool _xmltoarg(char *strxmlbuffer);

// 判断URL中的用户名和密码，如果不正确，放回认证失败的响应报文。线程退出
bool Login(connection* conn, const char* strrecvbuf, const int sockfd);

// 解析url并将解析内容存放在name里
// http://127.0.0.1:8080/api?username=xx&password=xx&inyrtname=getZHOBTMIND1&begintime=20220507123000&endtime=20220507123000
bool getvalue(const char* strget, const char* name, char *value, const int len);

// 判断用户是否有调用接口的权限，如果没有，放回没有权限的响应报文，线程退出
bool CheckPerm(connection* conn, const char* strrecvbuf, const int sockfd);

// 再执行接口的sql语句，把数据放回给客户端
bool ExecSQL(connection* conn, const char* strrecvbuf, const int sockfd);

int main(int argc,char *argv[])
{
	if (argc!=3) { _help(argv); return -1; }

    // 关闭全部的信号和输入输出。
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程
    // 但请不要用 "kill -9 +进程号" 强行终止
    CloseIOAndSignal(); signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    if (logfile.Open(argv[1],"a+")==false) { printf("logfile.Open(%s) failed.\n",argv[1]); return -1; }

    // 把xml解析到参数starg结构中
    if (_xmltoarg(argv[2])==false) return -1;

    // 服务端初始化。
    if (TcpServer.InitServer(starg.port)==false)
    {
        logfile.Write("TcpServer.InitServer(%d) failed.\n",starg.port); return -1;
    }

    // 初始化数据库连接池
    if(initconns() ==  false)
    {
        logfile.Write("数据库连接池初始化失败\n");
        return -1;
    }

	pthread_spin_init(&spin, 0);		// 初始化锁

	while (1)
	{
		// 等待客户端的连接请求。
		if (TcpServer.Accept()==false)
		{
			logfile.Write("TcpServer.Accept() failed.\n");
			EXIT(-1);
		}

		logfile.Write("客户端（%s）已连接。\n",TcpServer.GetIP());

		// 创建一个新的线程，让他和客户端进行通讯
		pthread_t thid = 0;
		if(pthread_create(&thid, NULL, thmain, (void*)(long)TcpServer.m_connfd) != 0)
		{
			logfile.Write("线程创建失败\n");
			TcpServer.CloseListen();
			continue;
		}
		pthread_spin_lock(&spin);		// 加锁
		vpid.push_back(thid);
		pthread_spin_unlock(&spin);		// 解锁
	}

	return 0;
  
}

void EXIT(int sig)   // 线程退出函数
{
	// 忽略信号,防止干扰
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	// 先关闭监听套接字
	TcpServer.CloseListen();

	// 取消全部的线程
    pthread_spin_lock(&spin);
	for(auto iter = vpid.begin(); iter != vpid.end(); ++iter)
	{
		pthread_cancel(*iter);
	}
    pthread_spin_unlock(&spin);

	sleep(1);		// 休息1s。保证线程清理函数能够被调用

	// 释放锁
	pthread_spin_destroy(&spin);

	exit(0);
}

void* thmain(void *arg)		// 线程入口函数
{
	int connfd = (int)(long)arg;			// 客户端的socket

	pthread_cleanup_push(thcleanup, arg);		// 线程清理函数入栈

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);		// 把线程取消方式设置为立即取消

	pthread_detach(pthread_self());		// 然后将线程分离出去

    char strrecvbuf[1024];      // 接收报文的buffer

    // 读取客户端的报文，如果超时或者失败，线程退出
    memset(strrecvbuf, 0, sizeof(strrecvbuf));
    if(ReadT(connfd, strrecvbuf, sizeof(strrecvbuf), 3) <= 0)
    {
        pthread_exit(0);
    }

    // 如果不是GET请求，报文不进行处理，线程退出
    if(strncmp(strrecvbuf, "GET", 3) != 0)
    {
        pthread_exit(0);
    }

    logfile.Write("%s\n", strrecvbuf);

    // 连接数据库
    connection *conn = getconns();

    // 判断URL中的用户名和密码，如果不正确，放回认证失败的响应报文。线程退出
    if(Login(conn, strrecvbuf, connfd) == false)
    {
        freecons(conn);
        pthread_exit(0);
    }

    // 判断用户是否有调用接口的权限，如果没有，放回没有权限的响应报文，线程退出
    if(CheckPerm(conn, strrecvbuf, connfd) == false)
    {
        freecons(conn);
        pthread_exit(0);
    }

    // 先把响应报文的头部发送给客户端
    char strsendbuf[1024];
    memset(strsendbuf, 0, sizeof(strsendbuf));
    sprintf(strsendbuf, \
            "HTTP/1.1 200 OK\r\n"\
            "Server:webserver\r\n"\
            "Content-Type:text/html;charset=utf-8\r\n\r\n");

    Writen(connfd, strsendbuf, strlen(strsendbuf));

    // 再执行接口的sql语句，把数据放回给客户端
    if(ExecSQL(conn, strrecvbuf, connfd) == false)
    {
        freecons(conn);
        pthread_exit(0);
    }

    freecons(conn);

	pthread_cleanup_pop(1);

	return NULL;
}

void thcleanup(void *arg)		// 线程清理函数
{
	int conndfd = (int)(long)arg;

	close(conndfd);

    pthread_spin_lock(&spin);
	// 把本线程的id从容器中删除
	for(auto iter = vpid.begin(); iter != vpid.end(); ++iter)
	{
		if(pthread_equal(*iter, pthread_self()) == 0)
		{
			vpid.erase(iter);
			break;
		}
	}
	pthread_spin_unlock(&spin);

	logfile.Write("线程%lu退出\n", pthread_self());
}

// 显示程序的帮助
void _help(char *argv[])
{
    printf("Using:/project/tools/bin/webserver logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 10 /project/tools/bin/webserver /log/idc/webserver.log \"<connstr>invi/password@snorcl11g_130</connstr><charset>Simplified Chinese_China.AL32UTF8</charset><port>8080</port>\"\n\n");

    printf("本程序是数据总线的服务端程序，为数据中心提供http协议的数据访问接口。\n");
    printf("logfilename 本程序运行的日志文件。\n");
    printf("xmlbuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据库的连接参数，格式：username/password@tnsname。\n");
    printf("charset     数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
    printf("port        web服务监听的端口。\n\n");
}

// 把xml解析到参数starg结构中
bool _xmltoarg(char *strxmlbuffer)
{
    memset(&starg,0,sizeof(struct st_arg));

    GetXMLBuffer(strxmlbuffer,"connstr",starg.connstr,100);
    if (strlen(starg.connstr)==0) { logfile.Write("connstr is null.\n"); return false; }

    GetXMLBuffer(strxmlbuffer,"charset",starg.charset,50);
    if (strlen(starg.charset)==0) { logfile.Write("charset is null.\n"); return false; }

    GetXMLBuffer(strxmlbuffer,"port",&starg.port);
    if (starg.port==0) { logfile.Write("port is null.\n"); return false; }

    return true;
}

// 读取客户端的报文
int ReadT(const int sockfd, char* buffer, const int size, const int itimeout)
{
    if(itimeout > 0)
    {
        struct pollfd fds;
        
        fds.fd = sockfd;
        fds.events = POLLIN;
        int iret;

        if((iret = poll(&fds, 1, itimeout*1000)) <= 0)
        {
            return false;
        }
    }

    return recv(sockfd, buffer, size, 0);
}

// 判断URL中的用户名和密码，如果不正确，放回认证失败的响应报文。线程退出
bool Login(connection* conn, const char* strrecvbuf, const int sockfd)
{
    char username[31],passwd[31];

    getvalue(strrecvbuf, "username", username, 30);     // 获取用户名
    getvalue(strrecvbuf, "passwd", passwd, 30);         // 获取密码

    // 查询T_USERINFO表，判断用户名和密码是否存在
    sqlstatement stmt;
    stmt.connect(conn);
    stmt.prepare("select count(*) from T_USERINFO where username=:1 and passwd=:2 and rsts=1");
    stmt.bindin(1, username, 30);
    stmt.bindin(2, passwd, 30);
    
    int icount = 0;
    stmt.bindout(1, &icount);
    stmt.execute();
    stmt.next();

    if(icount == 0)     // 认证失败，放回认证失败的响应报文
    {
        char strbuffer[256];
        memset(strbuffer, 0, sizeof(strbuffer));

        sprintf(strbuffer, \
                "HTTP/1.1 200 OK\r\n"\
                "Server:webserver\r\n"\
                "Content-Type:text/html;charset=utf-8\r\n\r\n"\
                "<retcode>-1</retcode><message>username or passwd is invalied</message>");

        Writen(sockfd, strbuffer, strlen(strbuffer));

        return false;
    }

    return true;
}

bool getvalue(const char* strget, const char* name, char *value, const int len)
{

	value[0] = 0;

	char *start,*end;
	start=end=0;

	start=strstr((char*)strget, (char*)name);

	if(start == 0) return false;

	end = strstr(start, "&");

	if(end == 0) end = strstr(start, " ");

	if(end == 0) return false;

	int ilen = end - (start+strlen(name)+1);
	if(ilen>len) ilen = len;

	strncpy(value, start+strlen(name)+1, ilen);

	value[ilen] = 0;

	return true;
}

// 判断用户是否有调用接口的权限，如果没有，放回没有权限的响应报文，线程退出
bool CheckPerm(connection* conn, const char* strrecvbuf, const int sockfd)
{
    char username[31],intername[30];

    getvalue(strrecvbuf,"username",username,30);    // 获取用户名。
    getvalue(strrecvbuf,"intername",intername,30);  // 获取接口名。

    sqlstatement stmt;
    stmt.connect(conn);
    stmt.prepare("select count(*) from T_USERANDINTER where username=:1 and intername=:2 and intername in (select intername from T_INTERCFG where rsts=1)");
    stmt.bindin(1,username,30);
    stmt.bindin(2,intername,30);
    int icount=0;
    stmt.bindout(1,&icount);
    stmt.execute();
    stmt.next();

    if (icount!=1)
    {
        char strbuffer[256];
        memset(strbuffer,0,sizeof(strbuffer));

        sprintf(strbuffer,\
            "HTTP/1.1 200 OK\r\n"\
            "Server: webserver\r\n"\
            "Content-Type: text/html;charset=utf-8\r\n\n\n"\
            "<retcode>-1</retcode><message>permission denied</message>");

        Writen(sockfd,strbuffer,strlen(strbuffer));

        return false;
    }

    return true;
}

// 再执行接口的sql语句，把数据放回给客户端
bool ExecSQL(connection* conn, const char* strrecvbuf, const int sockfd)
{
    // 从请求报文中解析接口名
    char intername[31];
    memset(intername, 0,sizeof(intername));
    getvalue(strrecvbuf, "intername", intername, 30);       // 获取接口名

    // 从接口参数配置表T_INTERCFG中加载接口参数
    char selectsql[1001],colstr[301],bindin[301];
    memset(selectsql, 0, sizeof(selectsql));        // 接口sql
    memset(colstr, 0, sizeof(colstr));              // 输出列名
    memset(bindin, 0, sizeof(bindin));              // 接口参数

    sqlstatement stmt;
    stmt.connect(conn);
    stmt.prepare("select selectsql,colstr,bindin from T_INTERCFG where intername = :1");
    stmt.bindin(1, intername, 30);              // 接口名
    stmt.bindout(1, selectsql, 1000);           // 接口sql
    stmt.bindout(2, colstr, 300);               // 输出列名
    stmt.bindout(3, bindin, 300);                // 接口参数

    stmt.execute();        // 这里基本上不用判断返回值，出错的几率几乎没有
    stmt.next();

    // 准备查询数据的SQL语句
    stmt.prepare(selectsql);

    // http://192.168.174.132:8080?username=ty&passwd=typwd&intername=getzhobtmind3&obtid=59287&begintime=20211024094318&endtime=20211024113920
    // SQL语句：   select obtid,to_char(ddatetime,'yyyymmddhh24miss'),t,p,u,wd,wf,r,vis from T_ZHOBTMIND where obtid=:1 and ddatetime>=to_date(:2,'yyyymmddhh24miss') and ddatetime<=to_date(:3,'yyyymmddhh24miss')
    // colstr字段：obtid,ddatetime,t,p,u,wd,wf,r,vis
    // bindin字段：obtid,begintime,endtime

    // 绑定查询数据的sql语句的输入变量
    // 根据接口配置中的参数列表（bindin字段），从URL中解析出参数的值，绑定到查询数据的sql语句中
    // --------------------------------------------------------------
    // 拆分数据传输bindin
    CCmdStr Cmdstr;
    Cmdstr.SplitToCmd(bindin, ",");
    // 声明一个用于存放输入参数的数组，输入参数的值不会太长，100足够
    char invalue[Cmdstr.CmdCount()][101];
    memset(invalue, 0, sizeof(invalue));
    // 从http的GET请求报文中解析出输入参数，绑定到sql中
    for(int i = 0; i<Cmdstr.CmdCount();i++)
    {
        getvalue(strrecvbuf, Cmdstr.m_vCmdStr[i].c_str(), invalue[i], 100);
        stmt.bindin(i+1, invalue[i], 100);
    }
    // --------------------------------------------------------------

    // 绑定存数据的sql语句的输出变量
    // 根据接口配置中的列名（colstr字段），binout结果集
    // --------------------------------------------------------------
    // 拆分结果集的字段名colstr，得到结果集的字段数
    Cmdstr.SplitToCmd(colstr, ",");
    // 声明一个数组，用于存放结果集的数组
    char colvalue[Cmdstr.CmdCount()][2001];
    memset(colvalue, 0, sizeof(colvalue));
    // 把结果集绑定到colvalue数组中
    for(int i =0; i < Cmdstr.CmdCount(); i++)
    {
        stmt.bindout(i+1, colvalue[i], 2000);
    }
    // --------------------------------------------------------------

    // 执行sql语句
    char strsendbuffer[4001];           // 发送给客户端的xml
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    if(stmt.execute() !=0)
    {
        logfile.Write("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message);
        sprintf(strsendbuffer, "<retcode>%d</retcode><message>%s</message>\n", stmt.m_cda.rc, stmt.m_cda.message);
        Writen(sockfd, strsendbuffer, strlen(strsendbuffer));
        return false;
    }
    strcpy(strsendbuffer, "<retcode>0</retcode><message>ok</message>\n");
    Writen(sockfd, strsendbuffer, strlen(strsendbuffer));

    // 向客户端发送xml内容的头部标签<data>
    Writen(sockfd, "<data>\n", strlen("<data>\n"));

    // 获取结果集，每获取一条记录，就拼接xml报文，发送给客户端
    char strtemp[2001];                 // 用于拼接xml的临时变量
    while (true)
    {
        memset(strsendbuffer, 0, sizeof(strsendbuffer));
        memset(strtemp, 0, sizeof(strtemp));

        if(stmt.next() != 0) break;     // 从结果集中获取一条记录

        // 拼接每个字段的xml
        for(int i = 0; i < Cmdstr.CmdCount(); i++)
        {
            memset(strtemp, 0, sizeof(strtemp));
            snprintf(strtemp, 2000, "<%s>%s</%s>", Cmdstr.m_vCmdStr[i].c_str(), colvalue[i], Cmdstr.m_vCmdStr[i].c_str());
            strcat(strsendbuffer, strtemp);
        }

        strcat(strsendbuffer, "<endl/>\n");         // xml每行的结束标志

        Writen(sockfd, strsendbuffer, strlen(strsendbuffer));       // 向客户端返回这行数据
    }
    

    // 向客户端发送xml内容的尾部标签</data>
    Writen(sockfd, "</data>\n", strlen("</data>\n"));

    // 写接口调用日志表T_USERLOG
    logfile.Write("intername=%s, count=%d\n", intername, stmt.m_cda.rpc);

    return true;
}

bool initconns()                   // 初始化数据库连接池
{
    for(int i = 0; i<MAXCONNS; i++)
    {
        // 连接数据库
        if(conns[i].connecttodb(starg.connstr, starg.charset) != 0)
        {
            logfile.Write("connect to database(%s) failed\n%s\n", starg.connstr, conns[i].m_cda.message);
            return false;
        }

        // 初始化互斥锁
        pthread_mutex_init(&mutex[i], 0);

    }

    return true;
}

connection *getconns()             // 从数据库连接池中获取一个空闲的连接
{
    // 采用轮询的方法，对互斥锁进行加锁，如果加锁成功，就返回这把锁对应的数据库连接
    while (true)
    {
        for(int i = 0; i<MAXCONNS; i++)
        {
            if(pthread_mutex_lock(&mutex[i]) == 0)
            {
                logfile.Write("get conns is %d.\n", i);
                return &conns[i];
            }
        }
        usleep(10000);      // 百分之1s之后再重试
    }
    
}

bool freecons(connection* conn)    // 释放、归还 数据库连接
{
    for(int i = 0; i<MAXCONNS; i++)
    {
        if(&conns[i] == conn)
        {
            pthread_mutex_unlock(&mutex[i]);
            conn = nullptr;
            logfile.Write("get conns is %d.\n", i);
            return true;
        }
    }

    return false;
}

bool destroyconns()                // 释放数据库连接占用的资源（断开数据库连接 并 销毁锁）
{
    for(int i = 0; i<MAXCONNS; i++)
    {
        conns[i].disconnect();      // 断开数据库连接
        pthread_mutex_destroy(&mutex[i]);       // 销毁互斥锁
    }

    return true;
}
