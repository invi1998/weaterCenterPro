/*
 * 程序名：demo14.cpp，此程序用于演示网银APP软件的服务端，增加了心跳报文。
 *
 * author：invi
*/
#include "../_public.h"

CLogFile logfile;   // 服务程序运行日志对象
CTcpServer TcpServer;   // tcp服务端类对象

// 记录用户是否登陆成功
bool loginS = false;

void FathEXIT(int sig);   // 父进程退出函数
void ChildEXIT(int sig);  // 子进程退出函数

bool _main(const char *strrecv, char *strsend);   // 业务处理子函数

bool srv000(const char *strrecv, char *strsend);    // 处理心跳报文

bool srv001(const char *strrecv, char *strsend);    // 登陆业务

bool srv002(const char *strrecv, char *strsend);    // 余额查询业务
 
int main(int argc,char *argv[])
{
  if (argc!=4)
  {
    printf("Using:./demo14 port logfile timeout\nExample:./demo14 5005 /tmp/demo14.log 35\n\n"); return -1;
  }

  // 关闭全部的信号和输入输出。
  // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程
  // 但请不要用 "kill -9 +进程号" 强行终止
  CloseIOAndSignal();
  signal(SIGINT,FathEXIT);
  signal(SIGTERM,FathEXIT);

  if(logfile.Open(argv[2], "a+") == false)
  {
    printf("logfile.Open(%s, \"a+\") failed\n", argv[2]);
    return -1;
  }


  // 服务端初始化。
  if (TcpServer.InitServer(atoi(argv[1]))==false)
  {
    logfile.Write("TcpServer.InitServer(%s) failed.\n",argv[1]);
    return -1;
  }

  while (1)
  {
    // 等待客户端的连接请求。
    if (TcpServer.Accept()==false)
    {
      logfile.Write("TcpServer.Accept() failed.\n");
      FathEXIT(-1);
    }

    logfile.Write("客户端（%s）已连接。\n",TcpServer.GetIP());

    if(fork() > 0)
    {
      TcpServer.CloseClient();    // 父进程中关闭连接套接字（client 客户端套接字）
      logfile.Write(" 父进程 listenfd = %d, connfd = %d \n", TcpServer.m_listenfd, TcpServer.m_connfd);
      continue;   // 父进程继续回到accept
    }

    // 设置子进程的退出信号
    signal(SIGINT,ChildEXIT);
    signal(SIGTERM,ChildEXIT);

    TcpServer.CloseListen();    // 然后再子进程中关闭监听套接字

    logfile.Write(" 子进程 listenfd = %d, connfd = %d \n", TcpServer.m_listenfd, TcpServer.m_connfd);


    // 下面这些流程就是子进程的分支（子进程走下来处理业务）
    char strrecvbuffer[1024];
    char strsendbuffer[1024];

    // 与客户端通讯，接收客户端发过来的报文后，回复ok。
    while (1)
    {
      memset(strrecvbuffer,0,sizeof(strrecvbuffer));
      memset(strsendbuffer,0,sizeof(strsendbuffer));

      if (TcpServer.Read(strrecvbuffer, atoi(argv[3]))==false) break; // 接收客户端的请求报文。
      logfile.Write("接收：%s\n",strrecvbuffer);

      // 业务处理子函数
      if(_main(strrecvbuffer, strsendbuffer) == false)
      {
        ChildEXIT(-1);
      }

      if (TcpServer.Write(strsendbuffer)==false) break; // 向客户端发送响应结果。
      logfile.Write("发送：%s\n",strsendbuffer);
    }

    ChildEXIT(0);
    
  }
  
}

void FathEXIT(int sig)   // 父进程退出函数
{
  // 忽略信号,防止干扰
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  // 先关闭监听套接字
  TcpServer.CloseListen();

  // 然后给所有的子进程发送退出信号
  kill(0, 15);

  // 然后退出
  logfile.Write("父进程(%d)退出, sig = %d！\n", getpid(), sig);
  exit(0);
}


void ChildEXIT(int sig)  // 子进程退出函数
{
  // 忽略信号,防止干扰
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  // 关闭当前客户端的套接字
  TcpServer.CloseClient();

  logfile.Write("子进程(%d)退出, sig = %d!\n", getpid(), sig);
  // 然后退出
  exit(0);
}

bool _main(const char *strrecv, char *strsend)   // 业务处理子函数
{
  // 解析xml获取服务代码
  int srvcode;
  GetXMLBuffer(strrecv, "srvcode", &srvcode);

  // 处理相应的业务
  switch (srvcode)
  {
  case 0:
    // 处理心跳报文
    srv000(strrecv, strsend);
    break;

  case 1:
    // 处理登陆服务
    srv001(strrecv, strsend);
    break;

  case 2:
    // 处理账户余额查询
    srv002(strrecv, strsend);
    break;
  
  default:
    // 业务代码不合法
    logfile.Write("业务代码（%d）不合法\n", srvcode);
    return false;
  }

  return true;
}

bool srv000(const char *strrecv, char *strsend)    // 处理心跳报文
{
  strcpy(strsend, "<retcode>0</retcode><message>成功</message>");

  return true;
}

bool srv001(const char *strrecv, char *strsend)    // 登陆业务
{
  char tel[12], password[21];
  GetXMLBuffer(strrecv, "tel", tel, 11);
  GetXMLBuffer(strrecv, "password", password, 20);

  if(strcmp(tel, "12345678901") == 0 && strcmp(password, "12345") == 0)
  {
    loginS = true;
    strcpy(strsend, "<retcode>0</retcode><message>登陆成功</message>");
  }
  else
  {
    loginS = false;
    strcpy(strsend, "<retcode>-1</retcode><message>账号密码错误</message>");
  }

  return true;
}

bool srv002(const char *strrecv, char *strsend)    // 余额查询业务
{
  if(loginS == false)
  {
    strcpy(strsend, "<retcode>-1</retcode><message>用户未登陆</message>");
  }
  char cardid[31];
  double ye;
  GetXMLBuffer(strrecv, "cardid", cardid, 30);

  if(strcmp(cardid, "6260000000001") == 0)
  {
    strcpy(strsend, "<retcode>0</retcode><message>成功</message><ye>3423.32</ye>");
  }
  else
  {
    strcpy(strsend, "<retcode>-1</retcode><message>错误</message>");
  }

  return true;
}
