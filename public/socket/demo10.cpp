/*
 * 程序名：demo10.cpp，此程序演示采用开发框架的CTcpServer类实现socket通讯多进程的服务端。
 * 1）在多进程的服务程序中，如果杀掉一个子进程，和这个子进程通讯的客户端会断开，但是，不
 *    会影响其它的子进程和客户端，也不会影响父进程。
 * 2）如果杀掉父进程，不会影响正在通讯中的子进程，但是，新的客户端无法建立连接。
 * 3）如果用killall+程序名，可以杀掉父进程和全部的子进程。
 *
 * 多进程网络服务端程序退出的三种情况：
 * 1）如果是子进程收到退出信号，该子进程断开与客户端连接的socket，然后退出。
 * 2）如果是父进程收到退出信号，父进程先关闭监听的socket，然后向全部的子进程发出退出信号。
 * 3）如果父子进程都收到退出信号，本质上与第2种情况相同。
 *
 * author：invi
*/
#include "../_public.h"

CLogFile logfile;   // 服务程序运行日志对象
CTcpServer TcpServer;   // tcp服务端类对象

void FathEXIT(int sig);   // 父进程退出函数
void ChildEXIT(int sig);  // 子进程退出函数
 
int main(int argc,char *argv[])
{
  if (argc!=3)
  {
    printf("Using:./demo10 port logfile\nExample:./demo10 5005 /tmp/demo10.log\n\n"); return -1;
  }

  // 关闭全部的信号和输入输出。
  // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程
  // 但请不要用 "kill -9 +进程号" 强行终止
  CloseIOAndSignal();
  signal(SIGINT,FathEXIT);
  signal(SIGTERM,FathEXIT);

  if(logfile.Open(argv[2], "a+") == false)
  {
    printf("logfile.Open(%s, \"a+\") faild\n", argv[2]);
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
    char buffer[102400];

    // 与客户端通讯，接收客户端发过来的报文后，回复ok。
    while (1)
    {
      memset(buffer,0,sizeof(buffer));
      if (TcpServer.Read(buffer)==false) break; // 接收客户端的请求报文。
      logfile.Write("接收：%s\n",buffer);

      strcpy(buffer,"ok");
      if (TcpServer.Write(buffer)==false) break; // 向客户端发送响应结果。
      logfile.Write("发送：%s\n",buffer);
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
