/*
 * 程序名：demo26.cpp，此程序用于演示采用TcpServer类实现socket通讯的服务端。
 * author：invi
*/
#include "../_public.h"

// 把html文件内容发送给客户端
bool sendhtmlFile(const int sockfd, const char * filename);
 
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

	// 先把响应报文头部发送给客户端
	// HTTP/1.1 200 OK
	// Date: Tue, 03 May 2022 04:06:01 GMT
	// Server: Apache
	// Last-Modified: Tue, 03 May 2022 02:30:48 GMT
	// ETag: "1ce38-5de124bf54502"
	// Accept-Ranges: bytes
	// Content-Length: 118355
	// Vary: Accept-Encoding
	// Content-Type: text/html
	// Set-Cookie: ray_leech_token=1651554534; path=/
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "HTTP/1.1 200 OK\r\n"\
				"Server: demo26\r\n"\
				"Content-Type: text/html;charset=utf-8"\
				"\r\n"
				);
				// "Content-Length: 118329\r\n"\

	// 先把头部信息发送给客户端
	if (Writen(TcpServer.m_connfd,buffer,strlen(buffer))== false) return -1;

	// 再把html文件的内容发送给客户端
	if(sendhtmlFile(TcpServer.m_connfd, "index.html") == false)
	{
		printf("error\n");
		return -1;
	};
}

// 把html文件内容发送给客户端
bool sendhtmlFile(const int sockfd, const char * filename)
{
	int bytes = 0;
	char buffer[5000];
	FILE *fp = NULL;

	if((fp = FOPEN(filename, "rb")) == NULL)
	{
		printf("111\n");
		return false;
	}

	while (true)
	{
		memset(buffer, 0, sizeof(buffer));

		if((bytes = fread(buffer, 1, 5000, fp)) == 0)
		{
			printf("222\n");
			break;
		}

		if(Writen(sockfd, buffer, bytes) == false)
		{
			printf("333\n");
			fclose(fp);
			fp = NULL;
			return false;
		}

	}
	fclose(fp);

	return true;
  
}
