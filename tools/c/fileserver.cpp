/*
 * 程序名：demo14.cpp，此程序用于演示网银APP软件的服务端，增加了心跳报文。
 *
 * author：invi
*/
#include "_public.h"

CLogFile logfile;   // 服务程序运行日志对象
CTcpServer TcpServer;   // tcp服务端类对象

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
  char pname[51];           // 进程名，建议用"tcpgetfiles_后缀"的方式。
} starg;

void FathEXIT(int sig);   // 父进程退出函数
void ChildEXIT(int sig);  // 子进程退出函数

char strrecvbuffer[1024];   // 发送报文的buffer。
char strsendbuffer[1024];   // 接收报文的buffer。

// 把xml解析到参数starg结构中。
bool _xmltoarg(char *strxmlbuffer);

// 登录业务处理函数。
bool ClientLogin();

// 上传文件的主函数。
void RecvFilesMain();

// 接收上传文件内容的函数
bool RecvFile(const int socketfd, char *filename, const char *mtime, int filesize);

int main(int argc,char *argv[])
{
  if (argc!=3)
  {
    printf("Using:./fileserver port logfile\nExample:./fileserver 5005 /log/idc/fileserver.log\n\n"); return -1;
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

    // if(fork() > 0)
    // {
    //   TcpServer.CloseClient();    // 父进程中关闭连接套接字（client 客户端套接字）
    //   continue;   // 父进程继续回到accept
    // }

    // // 设置子进程的退出信号
    // signal(SIGINT,ChildEXIT);
    // signal(SIGTERM,ChildEXIT);

    // TcpServer.CloseListen();    // 然后再子进程中关闭监听套接字

    // 子进程与客户端进行通信,处理业务

    // 处理登录客户端的登录报文
    if(ClientLogin() == false)
    {
      ChildEXIT(-1);
    }

    // 如果clienttype == 1,调用上传文件的子函数
    if(starg.clienttype == 1)
    {
      RecvFilesMain();
    }

    // 如果clienttype==2,调用下载文件的子函数

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

// 把xml解析到参数starg结构中
bool _xmltoarg(char *strxmlbuffer)
{
  memset(&starg,0,sizeof(struct st_arg));

  // 不需要对参数做合法性判断，客户端已经判断过了。
  GetXMLBuffer(strxmlbuffer,"clienttype",&starg.clienttype);
  GetXMLBuffer(strxmlbuffer,"ptype",&starg.ptype);
  GetXMLBuffer(strxmlbuffer,"clientpath",starg.clientpath);
  GetXMLBuffer(strxmlbuffer,"andchild",&starg.andchild);
  GetXMLBuffer(strxmlbuffer,"matchname",starg.matchname);
  GetXMLBuffer(strxmlbuffer,"srvpath",starg.srvpath);

  GetXMLBuffer(strxmlbuffer,"timetvl",&starg.timetvl);
  if (starg.timetvl>30) starg.timetvl=30;

  GetXMLBuffer(strxmlbuffer,"timeout",&starg.timeout);
  if (starg.timeout<50) starg.timeout=50;

  GetXMLBuffer(strxmlbuffer,"pname",starg.pname,50);
  strcat(starg.pname,"_srv");

  return true;
}

// 登录（这里的登录不是常规意义上的登陆，而是解析客户端发送的登陆报文，这个报文里面有服务端运行所需要的参数）
bool ClientLogin()
{
  memset(strrecvbuffer,0,sizeof(strrecvbuffer));
  memset(strsendbuffer,0,sizeof(strsendbuffer));

  if (TcpServer.Read(strrecvbuffer,20)==false)
  {
    logfile.Write("TcpServer.Read() failed.\n"); return false;
  }
  // logfile.Write("strrecvbuffer=%s\n",strrecvbuffer);

  // 解析客户端登录报文。
  _xmltoarg(strrecvbuffer);

  // 如果客户端发送的报文中，clienttype既不是1也不是2就是非法报文，合法报文回复OK，非法报文回复failed
  if(starg.clienttype != 1 && starg.clienttype != 2)
  {
    strcpy(strsendbuffer, "failed");
  }
  else
  {
    strcpy(strsendbuffer, "ok");
  }

  if(TcpServer.Write(strsendbuffer) == false)
  {
    logfile.Write("TcpServer.Write() failed.\n"); return false;
  }

  // 把登陆结果记录下来
  logfile.Write("%s login %s.\n",TcpServer.GetIP(),strsendbuffer);
  
  return true;

}

// 上传文件的主函数。
void RecvFilesMain()
{
  while (true)
  {
    memset(strsendbuffer,0,sizeof(strsendbuffer));
    memset(strrecvbuffer,0,sizeof(strrecvbuffer));

    // 接收客户端的报文。
    // 第二个参数（超时时间）的取值必须大于starg.timetvl，小于starg.timeout。
    // starg.timetvl扫描本地目录文件的时间间隔，单位：秒。
    // starg.timeout进程心跳的超时时间
    // 也就是你不能在它还在扫描文件的间隔期间就判定为超时，也不能已经超时超过了进程的心跳时间了还不认为是超时
    if (TcpServer.Read(strrecvbuffer,starg.timetvl+10)==false)
    {
      logfile.Write("TcpServer.Read() failed.\n"); return;
    }
    // logfile.Write("strrecvbuffer=%s\n",strrecvbuffer);

    // 处理心跳报文。
    if (strcmp(strrecvbuffer,"<activetest>ok</activetest>")==0)
    {
      strcpy(strsendbuffer,"ok");
      // logfile.Write("strsendbuffer=%s\n",strsendbuffer);
      if (TcpServer.Write(strsendbuffer)==false)
      {
        logfile.Write("TcpServer.Write() failed.\n"); return;
      }
    }

    // 处理上传文件的请求报文。上传报文他的开头10个字符一定是<filename>
    if(strncmp(strrecvbuffer, "<filename>", 10) == 0)
    {
      // 解析上传文件请求报文的xml
      char clientfilename[301];
      char mtime[21];
      int filesize = 0;

      memset(clientfilename, 0, sizeof(clientfilename));
      memset(mtime, 0, sizeof(mtime));

      GetXMLBuffer(strrecvbuffer, "filename", clientfilename, 300);
      GetXMLBuffer(strrecvbuffer, "mtime", mtime, 20);
      GetXMLBuffer(strrecvbuffer, "size", &filesize);

      // 客户端和服务端文件的目录是不一样的，一下代码用于生成服务端的文件名
      // 把文件名中的clientpath替换为srvpath。要注意第三个参数
      //       注意：
      // 1、如果str2比str1要长，替换后str会变长，所以必须保证str有足够的空间，否则内存会溢出。
      // 2、如果str2中包含了str1的内容，且bloop为true，这种做法存在逻辑错误，UpdateStr将什么也不做
      char serverfilename[301];
      memset(serverfilename, 0, sizeof(serverfilename));
      strcpy(serverfilename, clientfilename);
      UpdateStr(serverfilename, starg.clientpath, starg.srvpath, false);

      // 接收上传文件的内容
      logfile.Write("recv %s(%d) ... ", serverfilename, filesize);
      if(RecvFile(TcpServer.m_connfd, serverfilename, mtime, filesize) == true)
      {
        logfile.WriteEx("ok\n");
        SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1000, "<filename>%s</filename><result>ok</result>", clientfilename);
      }
      else
      {
        logfile.WriteEx("failed\n");
        SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1000, "<filename>%s</filename><result>failed</result>", clientfilename);
      }

      // 把接收结果返回给客户端
      if(TcpServer.Write(strsendbuffer) == false)
      {
        logfile.Write("TcpServer.Write(%s) failed\n", strsendbuffer);
        return;
      }

    }
  }

}

// 接收上传文件内容的函数
bool RecvFile(const int socketfd, char *filename, const char *mtime, int filesize)
{
  // 生成临时文件名，等待这个文件全部接收完毕之后，再改名为正式文件
  char strfilenametmp[301];
  SNPRINTF(strfilenametmp, sizeof(strfilenametmp), 300, "%s.tmp", filename);

  int totalbytes = 0;   // 已经接收文件的总字节数
  int onread = 0;       // 本次打算接收的总字节数
  char buffer[1000];    // 接收文件内容的缓冲区
  FILE *fp = nullptr;

  // 创建临时文件
  if((fp = FOPEN(strfilenametmp, "wb")) == nullptr)
  {
    return false;
  }

  while(1)
  {
    memset(buffer, 0, sizeof(buffer));

    // 计算本次应该接收的字节数
    if(filesize - totalbytes > 1000) onread = 1000;
    else onread = filesize - totalbytes;

    // 接收文件内容
    if(Readn(socketfd, buffer, onread) == false)
    {
      fclose(fp);
      return false;
    }

    // 把接收到的内容写入文件
    fwrite(buffer, 1, onread, fp);

    // 计算已经接收到的总字节数，如果文件已经接收完毕，跳出循环
    totalbytes = totalbytes + onread;

    if(totalbytes == filesize)
    {
      break;
    }

  }

  // 关闭临时文件
  if(fp != nullptr)
  {
    fclose(fp);
  }

  // 重置文件时间(注意：这里如果没有重置文件时间，那么文件时间就是传输这次文件的时间， 文件传输时间是没有意义的，文件原始时间更有意义)
  UTime(strfilenametmp, mtime);

  // 把临时文件RENAME为正式文件
  if(RENAME(strfilenametmp, filename) == false)
  {
    return false;
  }

  return true;
}
