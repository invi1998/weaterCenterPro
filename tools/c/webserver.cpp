/*
 * 程序名：webserver.cpp，此程序是数据服务总线的服务端程序。

 * author：invi
*/
#include "_public.h"
#include "_ooci.h"

CLogFile logfile;   // 服务程序运行日志对象
CTcpServer TcpServer;   // tcp服务端类对象

struct st_arg
{
    char connstr[101];      // 数据库连接参数
    char charset[51];       // 数据库连接的字符集
    int port;               // web服务监听端口
} starg;

// 线程信息结构体
struct st_pthinfo
{
    pthread_t pthid;            // 线程编号
    time_t  atime;              // 最近一次心跳时间
};

// 守护线程的线程id
pthread_t checkthid;
// 数据库连接池的监控线程id
pthread_t conndthid;

void EXIT(int sig);   // 线程退出函数

void* thmain(void *arg);		// 线程入口函数

void thcleanup(void *arg);		// 线程清理函数

pthread_spinlock_t spin;		// 自旋锁（对线程id容器这个公共资源进行加锁）
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;      // 定义互斥锁并初始化，用于实现生产消费者模型（给客户端socket队列做锁）
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;         // 定义并初始化条件变量

vector<int> sockqueue;          // 客户端socket队列

// 存放线程id
vector<st_pthinfo> vthread;

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

// 检测数据库连接池状态的函数
void* checkpool(void* args);

// 守护线程（监控线程）主函数
void* checkthmain(void *arg);

// 数据库连接池类
class connpool
{
private:
    struct st_conn
    {
        connection conn;    // 数据库连接
        pthread_mutex_t mutex;  // 数据库连接互斥锁
        time_t atime;           // 数据库连接上次使用的时间，如果未连接则取值为0
    }*m_conns;           // 数据库连接池

    int m_maxconns;     // 数据库连接池的最大值
    int m_timeout;      // 数据库连接的超时参数，单位：s
    char m_connstr[101];    // 数据库连接参数，用户名。密码@连接名
    char m_charset[101];    // 数据库字符集

public:
    connpool();
    ~connpool();

    // 初始化数据库连接池，初始化锁，如果数据库连接参数有问题，返回false
    bool init( char* connstr, char* charset, int maxcount, int timeout);

    // 断开数据库连接，销毁锁，释放数据库连接池的内存空间
    void destroy();

    // 从数据库连接池中获取一个空闲连接，成功放回数据库的连接地址
    // 如果连接池已经用完或者连接数据库失败，返回空
    connection *get();

    // 归还数据库连接
    bool free(connection *conn);

    // 检测数据库连接池，断开空闲连接(在服务程序中，用一个专用的子线程调用此函数)
    void checkpool();

};

connpool oracleconnpool;        // 声明连接池对象

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
    if(oracleconnpool.init(starg.connstr, starg.charset, 10, 50) ==  false)
    {
        logfile.Write("数据库连接池初始化失败\n");
        return -1;
    }
    else
    {
        // 创建一个线程，用来检测数据库连接池状态
        if(pthread_create(&conndthid, NULL, checkpool, 0) != 0)
        {
            logfile.Write("线程创建失败\n");
            return -1;
        }
    }

    // 创建守护线程
    if(pthread_create(&checkthid, NULL, checkthmain, 0) != 0)
    {
        logfile.Write("线程创建失败\n");
        return -1;
    }

    // 创建10个工作线程，线程数比CPU核数略多
    for(int i = 10; i < 10; i++)
    {
        pthread_t thid = 0;
        st_pthinfo thinfo;
		if(pthread_create(&thinfo.pthid, NULL, thmain, (void*)(long)i) != 0)
		{
			logfile.Write("线程创建失败\n");
			return -1;
		}

        thinfo.atime = time(0);         // 设置线程心跳时间为当前时间

        vthread.push_back(thinfo);       // 把线程ID保存到容器中
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

        // 把客户端的socket放入队列中，并发送条件信号
        pthread_mutex_lock(&mutex);                     // 加锁
        sockqueue.push_back(TcpServer.m_connfd);        // 把客户端的socket放入队列容器中
        pthread_mutex_unlock(&mutex);                   // 解锁
        pthread_cond_signal(&cond);                     // 触发条件，激活一个线程

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
	for(auto iter = vthread.begin(); iter != vthread.end(); ++iter)
	{
		pthread_cancel((*iter).pthid);
	}
    pthread_spin_unlock(&spin);

	sleep(1);		// 休息1s。保证线程清理函数能够被调用

    pthread_cancel(checkthid);      // 取消监控线程
    pthread_cancel(conndthid);      // 取消数据库连接池的监控线程

	// 销毁锁，销毁条件变量
	pthread_spin_destroy(&spin);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

	exit(0);
}

void* thmain(void *arg)		// 线程入口函数
{
	int pthnum = (int)(long)arg;			// 线程编号
    int connfd;                             // 客户端的socket

	pthread_cleanup_push(thcleanup, arg);		// 线程清理函数入栈

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);		// 把线程取消方式设置为立即取消

	pthread_detach(pthread_self());		// 然后将线程分离出去

    char strrecvbuf[1024];      // 接收报文的buffer
    char strsendbuf[1024];      // 发送报文的buffer

    while (true)
    {
        pthread_mutex_lock(&mutex);         // 给客户端socket队列加锁

        // 如果客户端socket队列为空，那就等待，用while防止条件变量虚假唤醒
        while (sockqueue.size() == 0)
        {
            struct timeval now;
            gettimeofday(&now, NULL);       // 获取当前时间
            now.tv_sec =  now.tv_sec+20;    // 把当前时间加20s
            pthread_cond_timedwait(&cond, &mutex, (timespec*)&now);

            // 上述这些代码表示，如果条件变量在20s之内没有被触发，那么就会超时返回，就会走到下面这里，就不会继续卡住，
            // 那么我们就再下面更新一下该线程的心跳信息
            vthread[pthnum].atime = time(0);
        }

        // 从客户端socket队列中取第一条记录，然后删除该记录
        connfd = sockqueue[0];
        sockqueue.erase(sockqueue.begin());

        pthread_mutex_unlock(&mutex);           // 给客户端socket队列解锁

        // 以下是业务处理代码

        logfile.Write("pthid = %lu(num = %d), socketid = %d\n", pthread_self(), pthnum, connfd);

        // 读取客户端的报文，如果超时或者失败，关闭客户端的socket，然后继续循环
        if(ReadT(connfd, strrecvbuf, sizeof(strrecvbuf), 3) <= 0)
        {
            close(connfd);
            continue;
        }

        // 如果不是GET请求，报文不进行处理，关闭客户端的socket，然后继续循环
        memset(strrecvbuf, 0, sizeof(strrecvbuf));
        if(strncmp(strrecvbuf, "GET", 3) != 0)
        {
            close(connfd);
            continue;
        }

        logfile.Write("%s\n", strrecvbuf);

        // 连接数据库
        connection *conn = oracleconnpool.get();

        // 如果数据库连接为空，向客户端返回系统错误，关闭这个socket，然后继续循环
        if(conn == nullptr)
        {
            memset(strsendbuf, 0, sizeof(strsendbuf));
            sprintf(strsendbuf, \
                    "HTTP/1.1 200 OK\r\n"\
                    "Server:webserver\r\n"\
                    "Content-Type:text/html;charset=utf-8\r\n\r\n"\
                    "<retcode>-1</retcode><message>系统错误</message>");

            Writen(connfd, strsendbuf, strlen(strsendbuf));

            close(connfd);
            continue;
        }

        // 判断URL中的用户名和密码，如果不正确，放回认证失败的响应报文。关闭客户端的socket，然后继续循环
        if(Login(conn, strrecvbuf, connfd) == false)
        {
            oracleconnpool.free(conn);
            close(connfd);
            continue;
        }

        // 判断用户是否有调用接口的权限，如果没有，放回没有权限的响应报文，关闭客户端的socket，然后继续循环
        if(CheckPerm(conn, strrecvbuf, connfd) == false)
        {
            oracleconnpool.free(conn);
            close(connfd);
            continue;
        }

        // 先把响应报文的头部发送给客户端
        memset(strsendbuf, 0, sizeof(strsendbuf));
        sprintf(strsendbuf, \
                "HTTP/1.1 200 OK\r\n"\
                "Server:webserver\r\n"\
                "Content-Type:text/html;charset=utf-8\r\n\r\n");

        Writen(connfd, strsendbuf, strlen(strsendbuf));

        // 再执行接口的sql语句，把数据放回给客户端, 关闭客户端的socket，然后继续循环
        if(ExecSQL(conn, strrecvbuf, connfd) == false)
        {
            oracleconnpool.free(conn);
            close(connfd);
            continue;
        }

        oracleconnpool.free(conn);
        
    }

    pthread_cleanup_pop(1);

    return NULL;
   
}

void thcleanup(void *arg)		// 线程清理函数
{
	int conndfd = (int)(long)arg;

    // 在生产消费者模型的线程清理函数中，要解锁互斥量，不让线程退出不了
    pthread_mutex_unlock(&mutex);

    // 把本线程id从存放线程id的容器中删除

    pthread_spin_lock(&spin);
	// 把本线程的id从容器中删除
	for(auto iter = vthread.begin(); iter != vthread.end(); ++iter)
	{
		if(pthread_equal((*iter).pthid, pthread_self()) == 0)
		{
			vthread.erase(iter);
			break;
		}
	}
	pthread_spin_unlock(&spin);

	logfile.Write("线程%d(%lu)退出\n", (int)(long)arg, pthread_self());
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

// -----------------------------------------------------------------------------------------------
connpool::connpool()
{
    m_maxconns = 0;
    m_timeout = 0;
    memset(m_connstr, 0, sizeof(m_connstr));
    memset(m_charset, 0, sizeof(m_charset));
    m_conns = nullptr;
}

connpool::~connpool()
{
    destroy();
}

// 初始化数据库连接池，初始化锁，如果数据库连接参数有问题，返回false
bool connpool::init(char* connstr, char* charset, int maxcount, int timeout)
{
    // 尝试数据库，验证数据库连接参数是否正确
    connection conn;
    if(conn.connecttodb(connstr, charset) != 0)
    {
        logfile.Write("数据库连接失败。\n%s\n", conn.m_cda.message);
        return false;
    }

    conn.disconnect();

    strncpy(m_connstr, connstr, 100);
    strncpy(m_charset, charset, 100);
    m_maxconns = maxcount;
    m_timeout = timeout;

    // 分配数据库连接池的内存空间
    m_conns = new struct st_conn[m_maxconns];

    for(int i = 0; i< m_maxconns; i++)
    {
        pthread_mutex_init(&m_conns[i].mutex, 0);       // 初始化锁
        m_conns[i].atime = 0;                   // 数据库连接上次使用的时间初始化为0
    }

    return true;
}

// 断开数据库连接，销毁锁，释放数据库连接池的内存空间
void connpool::destroy()
{
    for(int i = 0; i < m_maxconns; i++)
    {
        m_conns[i].conn.disconnect();       // 断开数据库连接
        pthread_mutex_destroy(&m_conns[i].mutex);       // 销毁锁
    }
    delete []m_conns;
    m_conns = nullptr;

    memset(m_connstr, 0, sizeof(m_connstr));
    memset(m_charset, 0, sizeof(m_charset));

    m_maxconns = 0;
    m_timeout = 0;
}

// 从数据库连接池中获取一个空闲连接，成功放回数据库的连接地址
// 如果连接池已经用完或者连接数据库失败，返回空
// 1）从数据库连接池中寻找一个空闲的，已经连接好的connection，如果找到了，返回他的地址
// 2）如果没有找到，在连接池中找一个未连接数据库的connection，连接数据库，如果成功，返回connection的地址
// 3）如果第2）步找到了未连接数据库的connection，但是连接数据库失败，返回false
// 4）如果第2）步没有找到未连接数据库的connection，表示数据库连接池已经用完，也返回空
connection *connpool::get()
{
    int pos = -1;           // 用于记录第一个未连接数据库的数组位置

    for(int i = 0; i < m_maxconns; i++)
    {
        if(pthread_mutex_trylock(&m_conns[i].mutex) == 0)
        {
            if(m_conns[i].atime > 0)        // 如果数据库是已经连接状态
            {
                m_conns[i].atime = time(0);     // 把数据库连接的使用时间置为当前时间
                return &m_conns[i].conn;        // 返回数据库连接的地址
            }


            if(pos == -1)
            {
                pos = i;        // 记录第一个未连接数据库的数组位置
            }
            else
            {
                pthread_mutex_unlock(&m_conns[i].mutex);        // 如果不是已连接状态，需要释放锁
            }
        }
    }

    if(pos == -1)       // 表示连接池已经用完，返回空
    {
        return nullptr;
    }

    // 连接池没有用完，让m_conns[pos].conn连上数据库
    if(m_conns[pos].conn.connecttodb(m_connstr, m_charset) != 0)
    {
        pthread_mutex_unlock(&m_conns[pos].mutex);
        return nullptr;
    }
    m_conns[pos].atime = time(0);           // 把数据库连接的使用时间置为当前时间
    return &m_conns[pos].conn;

}

// 归还数据库连接
bool connpool::free(connection *conn)
{
    for(int i = 0; i < m_maxconns; i++)
    {
        if(&m_conns[i].conn == conn)
        {
            m_conns[i].atime = time(0);
            pthread_mutex_unlock(&m_conns[i].mutex);
            return true;
        }
    }

    return false;
}

// 检测数据库连接池，断开空闲连接(在服务程序中，用一个专用的子线程调用此函数)
void connpool::checkpool()
{
    for(int i = 0; i < m_maxconns; i++)
    {
        if(pthread_mutex_trylock(&m_conns[i].mutex) == 0)
        {
            if(m_conns[i].atime > 0)        // 如果数据库是已经连接状态
            {
                // 判断连接是否超时
                if(time(0) - m_conns[i].atime > m_timeout)
                {
                    m_conns[i].conn.disconnect();       // 断开数据库连接
                    m_conns[i].atime = 0;               // 重置数据库连接的使用时间
                }
                else
                {
                    // 如果连接没有超时，执行一次sql，验证连接是否有效，如果无效，断开它
                    // 如果网络断开了，或者数据库重启了，就需要重新连接数据库，在这里只需要断开连接就可以
                    // 重连工作交给get()函数
                    if(m_conns[i].conn.execute("select * from dual") != 0)
                    {
                        m_conns[i].conn.disconnect();       // 断开数据库连接
                        m_conns[i].atime = 0;               // 重置连接使用时间
                    }
                }
            }

            pthread_mutex_unlock(&m_conns[i].mutex);        // 释放锁
        }

        // 如果尝试加锁失败，表示数据库连接正在使用中，不必检测
    }

}

// 检测数据库连接池状态的函数
void* checkpool(void* args)
{
    while (true)
    {
        /* code */
        oracleconnpool.checkpool();
        sleep(30);
    }
    return NULL;
}

// 守护线程（监控线程）主函数
void* checkthmain(void *arg)
{
    while (true)
    {
        // 遍历工作线程id的容器，检查每隔工作线程是否超时
        for(auto iter = vthread.begin(); iter != vthread.end(); ++iter)
        {
            // 工作线程超时时间设置为20s，这里用25s做超时判断
            if((time(0) - (*iter).atime) > 25)
            {
                // 线程心跳超时。写日志
                logfile.Write("thread %d(%lu) timeout(%d)\n", distance(vthread.begin(), iter), (*iter).pthid, time(0) - (*iter).atime);
                // 取消已经超时的工作线程
                pthread_cancel((*iter).pthid);

                // 然后再重新创建工作线程
                if(pthread_create(&(*iter).pthid, NULL, thmain, (void*)(long)distance(vthread.begin(), iter)) != 0)
                {
                    logfile.Write("线程创建失败。进程退出\n");
                    EXIT(-1);
                }

                // 设置工作线程的心跳时间
                (*iter).atime = time(0);
            }
        }
        
        sleep(3);
    }
    
}
