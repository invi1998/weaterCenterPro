/*
 * 程序名：demo27.cpp，此程序用于演示采用TcpClient类演示http通讯的客户端。
 * author：invi
*/
#include "../_public.h"
 
int main(int argc,char *argv[])
{
	if (argc!=3)
	{
		printf("Using:./demo27 ip port\nExample:./demo27 www.baidu.com 80\n\n"); return -1;
	}

	CTcpClient TcpClient;

	// 向服务端发起连接请求。
	if (TcpClient.ConnectToServer(argv[1],atoi(argv[2]))==false)
	{
		printf("TcpClient.ConnectToServer(%s,%s) failed.\n",argv[1],argv[2]); return -1;
	}

	char buffer[102400];
	
	// 生成http请求报文(注意，请求报文每一块内容上都需要加上回车换行\r\n)
	// GET /lesson/546.html HTTP/1.1
	// Host: 192.168.31.133:8080
	sprintf(buffer, "GET / HTTP/1.1\r\n"
			"Host: %s:%s\r\n"
			"\r\n", argv[1], argv[2]
	);

	// 用原生的send函数把http报文发送给服务端
	send(TcpClient.m_connfd, buffer, strlen(buffer), 0);

	// 接收服务端返回的网页内容
	while (true)
	{
		/* code */
		memset(buffer, 0, sizeof(buffer));
		if(recv(TcpClient.m_connfd, buffer, 102400, 0) <= 0)
		{
			return -1;
		};
		printf("%s", buffer);
	}
	
}

