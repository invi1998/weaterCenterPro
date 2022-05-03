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
	// Connection: keep-alive
	// Upgrade-Insecure-Requests: 1
	// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/100.0.4896.127 Safari/537.36
	// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
	// Accept-Encoding: gzip, deflate
	// Accept-Language: zh-CN,zh;q=0.9
	sprintf(buffer, "GET / HTTP/1.1\r\n",
			"Host: %s:%s\r\n",
			"\r\n", argv[1], argv[2]
	);

	// 用原生的send函数把http报文发送给服务端
	send(TcpClient.m_connfd, buffer, strlen(buffer), 0);

	// 接收服务端返回的网页内容
	memset(buffer, 0, sizeof(buffer));
	recv(TcpClient.m_connfd, buffer, sizeof(buffer), 0);
	printf("%s", buffer);

}

