/*
 * 程序名：tcpputfiles.cpp，采用tcp协议，实现文件上传的客户端。
 * author：invi
*/
#include "_public.h"

CTcpClient TcpClient;   // 客户端tcp通讯对象
CLogFile logfile;   // 日志
CPActive PActive;   // 进程心跳

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
bool Login(const char* argv);    // 登陆业务

// 程序退出和信号2、15的处理函数。
void EXIT(int sig);

// 程序帮助文档方法
void _help();

// 发送报文的buffer
char strsendbuffer[1024];

// 接收报文的buffer
char strrecvbuffer[1024];

// 如果调用_tcpputfiles发送了文件，bocontinue为true，初始化为true
bool bcontinue = true;

// 解析xml到参数starg中
bool _xmltoarg(char * strxmlbuffer);

// 文件上传函数
bool _tcpputfiles();

// 文件内容上传函数
bool SendFile(const int socketfd, char *filename, const int filesize);

// 删除或者转存到备份目录
bool AckMessage(const char* buffer);
 
int main(int argc,char *argv[])
{
  if (argc!=3) { _help(); return -1; }

  // 关闭全部的信号和输入输出。
  // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
  // 但请不要用 "kill -9 +进程号" 强行终止。
  CloseIOAndSignal(); signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

  // 打开日志文件
  if(logfile.Open(argv[1],  "a+") == false)
  {
    printf("logfile.Open(%s,  \"a+\") failed", argv[1]);
    return -1;
  }

  // 解析xml,得到程序的运行参数
  if(_xmltoarg(argv[2]) == false)
  {
    return -1;
  }

  // 把进程心跳写入共享内存
  PActive.AddPInfo(starg.timeout, starg.pname);

  // 向服务端发起连接请求。
  if (TcpClient.ConnectToServer(starg.ip, starg.port)==false)
  {
    logfile.Write("TcpClient.ConnectToServer(%s,%d) failed.\n",starg.ip, starg.port);
    EXIT(-1);
  }

  if(Login(argv[2]) == false)
  {
    logfile.Write("登陆失败！\n");
    EXIT(-1);
  }

  // 登陆成功后
  while (1)
  {
    // 调用文件上传的主函数，执行一次文件上传任务
    if(_tcpputfiles() == false)
    {
      logfile.Write("_tcpputfiles() failed\n");
      EXIT(-1);
    }

    if(bcontinue == false)
    {
      sleep(starg.timetvl);     // 休息这么多秒，也就是每隔timetvl就检测一次文件，看是否有新文件需要上传
      if(ActiveTest() == false) break;    // 休息结束发送一次心跳
    }

    PActive.UptATime();

  }

  EXIT(0);

}

bool ActiveTest()    // 心跳函数
{
  memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
  memset(strsendbuffer, 0, sizeof(strsendbuffer));

  SPRINTF(strsendbuffer, sizeof(strsendbuffer), "<activetest>ok</activetest>");
  // logfile.Write("发送：%s\n", strsendbuffer);

  if(TcpClient.Write(strsendbuffer) == false)   // 向服务端发送心跳报文
  {
    return false;
  }

  // 超时时间设置为20s
  if(TcpClient.Read(strrecvbuffer, 20) == false)    // 接收服务端的回应报文
  {
    return false;
  }
  // logfile.Write("接收：%s\n", strrecvbuffer);

  return true;
}

bool Login(const char *argv)    // 登陆业务
{
  memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
  memset(strsendbuffer, 0, sizeof(strsendbuffer));

  SPRINTF(strsendbuffer, sizeof(strsendbuffer), "%s<clienttype>1</clienttype>", argv);
  // logfile.Write("发送：%s\n", strsendbuffer);
  if(TcpClient.Write(strsendbuffer) == false)   // 向服务端发送请求报文
  {
    return false;
  }

  // 超时时间设置为20s
  if(TcpClient.Read(strsendbuffer, 20) == false)
  {
    return false;
  }
  // logfile.Write("接收：%s\n", strsendbuffer);

  logfile.Write("登陆%s:%d成功\n", starg.ip, starg.port);

  return true;
}

void _help()
{
  printf("\n");
  printf("Using:/project/tools/bin/tcpputfiles logfilename xmlbuffer\n\n");

  printf("Sample:/project/tools/bin/procctl 20 /project/tools/bin/tcpputfiles /log/idc/tcpputfiles_surfdata.log \"<ip>192.168.31.133</ip><port>5005</port><ptype>1</ptype><clientpath>/tmp/tcp/surfdata1</clientpath><clientpathbak>/tmp/tcp/surfdata1bak</clientpathbak><andchild>true</andchild><matchname>*.XML,*.CSV</matchname><srvpath>/tmp/tcp/surfdata2</srvpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_surfdata</pname>\"\n");
  printf("       /project/tools/bin/procctl 20 /project/tools/bin/tcpputfiles /log/idc/tcpputfiles_surfdata.log \"<ip>192.168.31.133</ip><port>5005</port><ptype>2</ptype><clientpath>/tmp/tcp/surfdata1</clientpath><clientpathbak>/tmp/tcp/surfdata1bak</clientpathbak><andchild>true</andchild><matchname>*.XML,*.CSV</matchname><srvpath>/tmp/tcp/surfdata2</srvpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_surfdata</pname>\"\n\n\n");

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

// 把xml解析到参数starg结构
bool _xmltoarg(char *strxmlbuffer)
{
  memset(&starg,0,sizeof(struct st_arg));

  GetXMLBuffer(strxmlbuffer,"ip",starg.ip);
  if (strlen(starg.ip)==0) { logfile.Write("ip is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"port",&starg.port);
  if ( starg.port==0) { logfile.Write("port is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"ptype",&starg.ptype);
  if ((starg.ptype!=1)&&(starg.ptype!=2)) { logfile.Write("ptype not in (1,2).\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"clientpath",starg.clientpath);
  if (strlen(starg.clientpath)==0) { logfile.Write("clientpath is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"clientpathbak",starg.clientpathbak);
  if ((starg.ptype==2)&&(strlen(starg.clientpathbak)==0)) { logfile.Write("clientpathbak is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"andchild",&starg.andchild);

  GetXMLBuffer(strxmlbuffer,"matchname",starg.matchname);
  if (strlen(starg.matchname)==0) { logfile.Write("matchname is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"srvpath",starg.srvpath);
  if (strlen(starg.srvpath)==0) { logfile.Write("srvpath is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"timetvl",&starg.timetvl);
  if (starg.timetvl==0) { logfile.Write("timetvl is null.\n"); return false; }

  // 扫描本地目录文件的时间间隔，单位：秒。
  // starg.timetvl没有必要超过30秒。
  if (starg.timetvl>30) starg.timetvl=30;

  // 进程心跳的超时时间，一定要大于starg.timetvl，没有必要小于50秒。
  GetXMLBuffer(strxmlbuffer,"timeout",&starg.timeout);
  if (starg.timeout==0) { logfile.Write("timeout is null.\n"); return false; }
  if (starg.timeout<50) starg.timeout=50;

  GetXMLBuffer(strxmlbuffer,"pname",starg.pname,50);
  if (strlen(starg.pname)==0) { logfile.Write("pname is null.\n"); return false; }

  return true;
}

// 文件上传函数,执行一次文件上传
bool _tcpputfiles()
{
  CDir Dir;
  // 调用OpenDir()打开starg.clientpath目录
  if(Dir.OpenDir(starg.clientpath, starg.matchname, 10000, starg.andchild) == false)
  {
    logfile.Write("Dir.OpenDir(%s, %s, 10000, true) failed\n", starg.clientpath, starg.matchname);
    return false;
  }

  // 定义一个变量，表示未收到确认报文的数量
  int delayed = 0;    // 每发送一个报文，该数量+1，每收到一个确认报文，该数量-1

  bcontinue = false;

  while(1)
  {
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    memset(strsendbuffer, 0, sizeof(strsendbuffer));

    // 遍历目录中的每个文件，调用ReadDir()获取一个文件名
    if(Dir.ReadDir() == false) break;

    // 只要有文件，就把变量 bcontinue 设置为true
    bcontinue = true;

    // 然后把文件名，文件修改时间，文件大小组成报文，发送给对端
    SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1000, "<filename>%s</filename><mtime>%s</mtime><size>%d</size>", Dir.m_FullFileName, Dir.m_ModifyTime, Dir.m_FileSize);

    // logfile.Write("strsendbuffer: %s\n", strsendbuffer);
    if(TcpClient.Write(strsendbuffer) == false)
    {
      logfile.Write("TcpClient.Write(%s) failed\n", strsendbuffer);
      return false;
    }

    // 把文件内容发送给对端
    logfile.Write("send %s(%d) ... ", Dir.m_FullFileName, Dir.m_FileSize);
    if(SendFile(TcpClient.m_connfd, Dir.m_FullFileName, Dir.m_FileSize) == true)
    {
      logfile.WriteEx("ok\n");
      delayed++;
    }
    else
    {
      logfile.WriteEx("failed\n");
      TcpClient.Close();
      return false;
    }

    PActive.UptATime();

    // 接收对端的确认报文
    while (delayed > 0)
    {
      memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
      if(TcpRead(TcpClient.m_connfd, strrecvbuffer, 0, -1) == false) break;

      // 删除或者转存到备份目录
      delayed--;
      AckMessage(strrecvbuffer);
    }
  }

  // 继续接收对端的确认报文
  while (delayed > 0)
  {
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    if(TcpRead(TcpClient.m_connfd, strrecvbuffer, 0, 10) == false) break;
    
    // 删除或者转存到备份目录
    delayed--;
    AckMessage(strrecvbuffer);
  }

  return true;
}

// 文件内容上传函数
bool SendFile(const int socketfd, char *filename, const int filesize)
{
  int onread = 0;   // 每次调用fread时打算读取的字节数
  int bytes = 0;    // 每次调用fread时从文件中读取的字节数
  char buffer[1000];    // 存放读取到数据的buffer
  int totalbytes = 0;       // 从文件中以读取的文件字节总数
  FILE *fp = NULL;

  // 打开文件，使用“rb"模式，因为我们要上传的文件除了文本数据，还有二进制的数据
  if((fp = fopen(filename, "rb")) == NULL)
  {
    return false;
  }

  // 同时如果文件比较大，不可能一次性把文件内容都读取出来，所以这里用一个循环，每次从文件中读取若干字节，然后发送
  while(true)
  {
    memset(buffer, 0, sizeof(buffer));
    // 计算本次应该读取的字节数，如果属于的数据超过1000字节，就打算读取1000字节。
    if(filesize - totalbytes > 1000) onread = 1000;
    else onread = filesize - totalbytes;

    // 然后从文件里读取数据
    bytes = fread(buffer, 1, onread, fp);   // bytes这个变量是成功读取的字节数

    // 把读取到的数据发送给对端
    if(bytes > 0)
    {
      // 注意这里发送使用框架里的Writen函数，如果使用TcpClient.Write函数，不一定能保证全部发送完毕
      if(Writen(socketfd, buffer, bytes) == false)
      {
        fclose(fp);
        return false;
      }
    }

    // 计算文件已经读取的字节数，如果文件已经读取完毕，就跳出循环
    totalbytes = totalbytes + bytes;

    if(totalbytes == filesize)    // 如果以读取的字节数等于文件大小，就表示已经读取完该文件的所有数据
    {
      break;
    }

  }

  if(fp != NULL)
  {
    fclose(fp);     // 关闭文件指针
  }

  return true;
}

// 删除或者转存到备份目录
bool AckMessage(const char* buffer)
{

  // 解析buffer
  char filename[301];
  char result[11];
  memset(filename, 0, sizeof(filename));
  memset(result, 0, sizeof(result));

  GetXMLBuffer(buffer, "filename", filename, 300);
  GetXMLBuffer(buffer, "result", result, 10);

  if(strcmp(result, "ok") != 0)
  {
    return true;
  }
  
  // ptype == 1.删除文件
  if(starg.ptype == 1)
  {
    if(REMOVE(filename) == false)
    {
      logfile.Write("REMOVE(%s) failed \n", filename);
      return false;
    }
  }

  // ptype == 2 .转存到备份目录
  if(starg.ptype == 2)
  {
    // 先生成备份目录的文件名
    char filenamebak[301];

    STRCPY(filenamebak, sizeof(filenamebak), filename);

    // 然后将备份目录的路径替换进去
    UpdateStr(filenamebak, starg.clientpath, starg.clientpathbak, false);

    if(RENAME(filename, filenamebak) == false)
    {
      logfile.Write("RENAME(%s, %s) failed\n", filename, filenamebak);
      return false;
    }
  }

  return true;
}
