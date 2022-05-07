/*
 * 程序名：demo26.cpp，此程序用于演示采用TcpServer类实现socket通讯的服务端。
 * author：invi
*/
#include "../_public.h"
#include "_ooci.h"

// 解析get请求中的参数，然后从数据库表T_ZHOBTMIND1中查询数据，返回给客户端
bool SendData(const int sockfd, const char * strget);
 
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

	// 客户端的请求报文
	char strget[102400];
	memset(strget, 0, sizeof(strget));

	// 接收http客户端发送过来的报文
	recv(TcpServer.m_connfd, strget, 1000, 0);

	printf("%s\n", strget);

	// 响应报文
	char strsend[102400];

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
	memset(strsend, 0, sizeof(strsend));
	sprintf(strsend, "HTTP/1.1 200 OK\r\n"\
				"Server: demo26\r\n"\
				"Content-Type: text/html;charset=utf-8\r\n"\
				"\r\n"
				);
				// "Content-Length: 118329\r\n"\

	// 先把头部信息发送给客户端
	if (Writen(TcpServer.m_connfd,strsend,strlen(strsend))== false) return -1;

	// 解析get请求中的参数，然后从数据库表T_ZHOBTMIND1中查询数据，返回给客户端
	if(SendData(TcpServer.m_connfd, strget) == false)
	{
		printf("error\n");
		return -1;
	};
}

// 解析get请求中的参数，然后从数据库表T_ZHOBTMIND1中查询数据，返回给客户端
bool SendData(const int sockfd, const char * strget)
{
	// 解析URL中的参数
	// 权限控制：用户名。密码
	// 接口名：访问数据的种类
	// 查询条件：设计接口的时候决定
	// http://127.0.0.1:8080/api?username=xx&password=xx&inyrtname=getZHOBTMIND1&begintime=20220507123000&endtime=20220507123000

	char username[31], passwd[31],intername[31], obtid[11],begintime[21],endtime[21];
	memset(username, 0, sizeof(username));
	memset(passwd, 0, sizeof(passwd));
	memset(intername, 0, sizeof(intername));
	memset(obtid, 0, sizeof(obtid));
	memset(begintime, 0, sizeof(begintime));
	memset(endtime, 0, sizeof(endtime));

	getvalue(strget, "username", username, 30);			// 获取用户名
	getvalue(strget, "passwd", passwd, 30);				// 获取密码
	getvalue(strget, "intername", intername, 30);		// 获取接口名
	getvalue(strget, "obtid", obtid, 10);				// 获取站点代码
	getvalue(strget, "begintime", begintime, 20);		// 获取起始时间
	getvalue(strget, "endtime", endtime, 20);			// 获取结束时间
	
	printf("username = %s\n", username);
	printf("passwd = %s\n", passwd);
	printf("intername = %s\n", intername);
	printf("obtid = %s\n", obtid);
	printf("begintime = %s\n", begintime);
	printf("endtime = %s\n", endtime);

	// 判断用户名、密码和接口名是否合法
	// 。。。。
	// 连接数据库
	connection conn;
	conn.connecttodb("invi/password@");

	return true;
  
}

// http://127.0.0.1:8080/api?username=xx&password=xx&inyrtname=getZHOBTMIND1&begintime=20220507123000&endtime=20220507123000
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
