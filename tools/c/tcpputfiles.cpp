/*
 * 程序名：tcpputfiles.cpp，采用tcp协议，实现文件上传的客户端。
 * author：invi
*/
#include "_public.h"

CTcpClient TcpClient;   // 客户端tcp通讯对象
CLogFile logfile;

// 程序运行的参数结构体。
struct st_arg
{
  int  clienttype;          // 客户端类型，1-上传文件；2-下载文件。
  char ip[31];              // 服务端的IP地址。
  int  port;                // 服务端的端口。
  int  ptype;               // 文件上传成功后文件的处理方式：1-删除文件；2-移动到备份目录。
  char clientpath[301];     // 本地文件存放的根目录。
  char clientpathbak[301];  // 文件成功上传后，本地文件备份的根目录，当ptype==2时有效。
  bool andchild;            // 是否上传clientpath目录下各级子目录的文件，true-是；false-否。
  char matchname[301];      // 待上传文件名的匹配规则，如"*.TXT,*.XML"。
  char srvpath[301];        // 服务端文件存放的根目录。
  int  timetvl;             // 扫描本地目录文件的时间间隔，单位：秒。
  int  timeout;             // 进程心跳的超时时间。
  char pname[51];           // 进程名，建议用"tcpputfiles_后缀"的方式。
} starg;

bool ActiveTest();    // tcp心跳
bool Login();    // 登陆业务

// 程序退出和信号2、15的处理函数。
void EXIT(int sig);

// 程序帮助文档方法
void _help();
 
int main(int argc,char *argv[])
{
  if (argc!=3) { _help(); return -1; }

  // 关闭全部的信号和输入输出。
  // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
  // 但请不要用 "kill -9 +进程号" 强行终止。
  CloseIOAndSignal(); signal(SIGINT,EXIT); signal(SIGTERM,EXIT);


  // 向服务端发起连接请求。
  if (TcpClient.ConnectToServer(argv[1],atoi(argv[2]))==false)
  {
    printf("TcpClient.ConnectToServer(%s,%s) failed.\n",argv[1],argv[2]); return -1;
  }

  if(Login() == false)
  {
    printf("登陆失败！\n");
    return -1;
  }


  return 0;

}

bool ActiveTest()    // 心跳函数
{
  char buffer[1024];
  
  SPRINTF(buffer, sizeof(buffer), "<srvcode>0</srvcode>");

  printf("发送：%s。。。\n", buffer);

  if(TcpClient.Write(buffer) == false)
  {
    return false;
  }

  memset(buffer, 0, sizeof(buffer));
  if(TcpClient.Read(buffer) == false)
  {
    return false;
  }
  printf("接收：%s\n", buffer);

  return true;
}

bool Login()    // 登陆业务
{
  char buffer[1024];
  
  SPRINTF(buffer, sizeof(buffer), "<srvcode>1</srvcode><tel>12345678901</tel><password>12345</password>");

  printf("发送：%s。。。\n", buffer);

  if(TcpClient.Write(buffer) == false)
  {
    return false;
  }

  memset(buffer, 0, sizeof(buffer));
  if(TcpClient.Read(buffer) == false)
  {
    return false;
  }

  // 解析服务端返回的xml
  int iretcode = -1;
  GetXMLBuffer(buffer, "retcode", &iretcode);

  if(iretcode != 0)
  {
    printf("登陆失败！\n");
    return false;
  }

  printf("登陆成功！\n");

  return true;
}

void _help()
{
  printf("\n");
  printf("Using:/project/tools1/bin/tcpputfiles logfilename xmlbuffer\n\n");

  printf("Sample:/project/tools1/bin/procctl 20 /project/tools1/bin/tcpputfiles /log/idc/tcpputfiles_surfdata.log \"<ip>192.168.174.132</ip><port>5005</port><ptype>1</ptype><clientpath>/tmp/tcp/surfdata1</clientpath><clientpathbak>/tmp/tcp/surfdata1bak</clientpathbak><andchild>true</andchild><matchname>*.XML,*.CSV</matchname><srvpath>/tmp/tcp/surfdata2</srvpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_surfdata</pname>\"\n");
  printf("       /project/tools1/bin/procctl 20 /project/tools1/bin/tcpputfiles /log/idc/tcpputfiles_surfdata.log \"<ip>192.168.174.132</ip><port>5005</port><ptype>2</ptype><clientpath>/tmp/tcp/surfdata1</clientpath><clientpathbak>/tmp/tcp/surfdata1bak</clientpathbak><andchild>true</andchild><matchname>*.XML,*.CSV</matchname><srvpath>/tmp/tcp/surfdata2</srvpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_surfdata</pname>\"\n\n\n");

  printf("本程序是数据中心的公共功能模块，采用tcp协议把文件发送给服务端。\n");
  printf("logfilename   本程序运行的日志文件。\n");
  printf("xmlbuffer     本程序运行的参数，如下：\n");
  printf("ip            服务端的IP地址。\n");
  printf("port          服务端的端口。\n");
  printf("ptype         文件上传成功后的处理方式：1-删除文件；2-移动到备份目录。\n");
  printf("clientpath    本地文件存放的根目录。\n");
  printf("clientpathbak 文件成功上传后，本地文件备份的根目录，当ptype==2时有效。\n");
  printf("andchild      是否上传clientpath目录下各级子目录的文件，true-是；false-否，缺省为false。\n");
  printf("matchname     待上传文件名的匹配规则，如\"*.TXT,*.XML\"\n");
  printf("srvpath       服务端文件存放的根目录。\n");
  printf("timetvl       扫描本地目录文件的时间间隔，单位：秒，取值在1-30之间。\n");
  printf("timeout       本程序的超时时间，单位：秒，视文件大小和网络带宽而定，建议设置50以上。\n");
  printf("pname         进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}

void EXIT(int sig)
{
  logfile.Write("程序退出，sig=%d\n\n",sig);

  exit(0);
}
