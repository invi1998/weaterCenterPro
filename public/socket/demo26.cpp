/*
 * 程序名：demo26.cpp，此程序用于演示采用TcpServer类实现socket通讯的服务端。
 * author：invi
*/
#include "../_public.h"
 
int main(int argc,char *argv[])
{
  if (argc!=2)
  {
    printf("Using:./demo26 port\nExample:./demo26 5005\n\n"); return -1;
  }

  CTcpServer TcpServer;

  // 服务端初始化。
  if (TcpServer.InitServer(atoi(argv[1]))==false)
  {
    printf("TcpServer.InitServer(%s) failed.\n",argv[1]); return -1;
  }

  // 等待客户端的连接请求。
  if (TcpServer.Accept()==false)
  {
    printf("TcpServer.Accept() failed.\n"); return -1;
  }

  printf("客户端（%s）已连接。\n",TcpServer.GetIP());

  char buffer[102400];
  memset(buffer, 0, sizeof(buffer));

  // 接收http客户端发送过来的报文
  recv(TcpServer.m_connfd, buffer, 1000, 0);

  printf("%s\n", buffer);
}
