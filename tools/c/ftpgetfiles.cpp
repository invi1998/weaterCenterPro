#include "_ftp.h"
#include "_public.h"

CLogFile logfile;

Cftp ftp;

CFile File;

// 程序运行参数结构体
struct st_arg
{
  char host[31];           // 远程服务器的IP和端口。
  int  mode;               // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
  char username[31];       // 远程服务器ftp的用户名。
  char password[31];       // 远程服务器ftp的密码。
  char remotepath[301];    // 远程服务器存放文件的目录。
  char localpath[301];     // 本地文件存放的目录。
  char matchname[101];     // 待下载文件匹配的规则。
  char listfilename[301];  // 下载前列出服务器文件名的文件。
} starg;

// 文件信息结构体
struct st_fileinfo
{
    char filename[301];   // 文件名。
    char mtime[21];       // 文件时间。
};

// 存储文件信息的容器，存放下载前列出的服务器文件名的容器
vector<struct st_fileinfo> vfilelist;

// 处理 2， 15的处理函数
void EXIT(int sig);

// 帮助文档
void _help();

// 解析第二个参数，（解析xml得到程序运行参数），将解析结果放到starg中
bool _xmltoarg(const char* args);

// 下载文件功能的主要函数
bool _ftpgetfiles();

// 把ftp.nlist()方法获取到的list文件加载到vfilelist中
bool LoadListFile();

int main(int argc, char *argv[])
{

    // 1：把服务器撒花姑娘某目录的文件全部下载到本地目录（可以指定文件名的匹配规则）
    // 日志文件名，ftp服务器的IP和端口，传输模式【主动|被动】ftp的用户名，ftp密码
    // 服务器存放文件的目录，蹦迪存放文件的目录，下载文件名匹配规则
    if(argc != 3)
    {
        _help();
        return -1;
    }

    // 把ftp服务上的某目录中的文件下载到本地目录中
    
    // 处理程序的退出信号
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
    // 但请不要用 "kill -9 +进程号" 强行终止。
    // CloseIOAndSignal(); 
    signal(SIGINT,EXIT);
    signal(SIGTERM,EXIT);

    // 打开日志文件
    if(logfile.Open(argv[1], "a+") == false)
    {
        printf("打开日志文件%s失败！\n", argv[1]);
        return -1;
    }

    // 解析xml。得到程序运行的参数
    if(_xmltoarg(argv[2]) == false)
    {
        return -1;
    }


    // 进入ftp服务存放文件的目录
    if(ftp.login(starg.host, starg.username, starg.password, starg.mode) == false)
    {
        logfile.Write("ftp.login(%s, %s, %s) faild\n", starg.host, starg.username, starg.password, starg.mode);
        return -1;
    }
    logfile.Write("登陆%s成功\n", starg.host);

    // 文件下载
    if(_ftpgetfiles() == false)
    {
        return -1;
    }

    // 退出ftp服务
    ftp.logout();

    return 0;
}

void EXIT(int sig)
{
    printf("程序退出， sig = %d\n", sig);

    exit(0);
}

void _help()
{
    printf("\n");
    printf("Using:/project/tools/bin/ftpgetfiles logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 30 /project/tools/bin/ftpgetfiles /log/idc/ftpgetfiles_surfdata.log \"<host>127.0.0.1:21</host><mode>1</mode><username>wucz</username><password>wuczpwd</password><localpath>/idcdata/surfdata</localpath><remotepath>/tmp/idc/surfdata</remotepath><matchname>SURF_ZH*.XML,SURF_ZH*.CSV</matchname><listfilename>/idcdata/ftplist/ftpgetfiles_surfdata.list</listfilename>\"\n\n\n");

    printf("本程序是通用的功能模块，用于把远程ftp服务器的文件下载到本地目录。\n");
    printf("logfilename是本程序运行的日志文件。\n");
    printf("xmlbuffer为文件下载的参数，如下：\n");
    printf("<host>127.0.0.1:21</host> 远程服务器的IP和端口。\n");
    printf("<mode>1</mode> 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。\n");
    printf("<username>wucz</username> 远程服务器ftp的用户名。\n");
    printf("<password>wuczpwd</password> 远程服务器ftp的密码。\n");
    printf("<remotepath>/tmp/idc/surfdata</remotepath> 远程服务器存放文件的目录。\n");
    printf("<localpath>/idcdata/surfdata</localpath> 本地文件存放的目录。\n");
    printf("<matchname>SURF_ZH*.XML,SURF_ZH*.CSV</matchname> 待下载文件匹配的规则。"\
         "不匹配的文件不会被下载，本字段尽可能设置精确，不建议用*匹配全部的文件。\n");
    printf("<listfilename>/idcdata/ftplist/ftpgetfiles_surfdata.list</listfilename> 下载前列出服务器文件名的文件。\n\n\n");
}

bool _xmltoarg(const char* args)
{
    memset(&starg, 0, sizeof(struct st_arg));
    GetXMLBuffer(args, "host", starg.host, 30);      // 远程ftp服务器的IP和端口
    if(strlen(starg.host) == 0)
    {
        logfile.Write("host is null\n");
        return false;
    }
    GetXMLBuffer(args, "mode", &starg.mode);       // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
    if(starg.mode != 2)
    {
        starg.mode = 1;     // 也就是说，如果没有指定ftp传输模式，那就默认都指定为被动模式
    }
    GetXMLBuffer(args, "username", starg.username, 30);      // 远程服务器ftp的用户名。
    if(strlen(starg.username) == 0)
    {
        logfile.Write("username is null\n");
        return false;
    }
    GetXMLBuffer(args, "password", starg.password, 30);       // 远程服务器ftp的密码。
    if(strlen(starg.password) == 0)
    {
        logfile.Write("password is null\n");
        return false;
    }
    GetXMLBuffer(args, "remotepath", starg.remotepath, 300);      // 远程服务器存放文件的目录
    if(strlen(starg.remotepath) == 0)
    {
        logfile.Write("remotepath is null\n");
        return false;
    }
    GetXMLBuffer(args, "localpath", starg.localpath, 300);      // 本地文件存放的目录
    if(strlen(starg.localpath) == 0)
    {
        logfile.Write("localpath is null\n");
        return false;
    }
    GetXMLBuffer(args, "matchname", starg.matchname, 100);     // 待下载文件匹配的规则
    if(strlen(starg.matchname) == 0)
    {
        logfile.Write("matchname is null\n");
        return false;
    }

    GetXMLBuffer(args,"listfilename",starg.listfilename,300);   // 下载前列出服务器文件名的文件。
    if (strlen(starg.listfilename)==0)
    {
        logfile.Write("listfilename is null.\n");
        return false;
    }

    return true;
}

bool _ftpgetfiles()
{
    // 调用ftp.nlist()方法列出服务器目录中的文件，结果存放在本地文件中。
    if(ftp.chdir(starg.remotepath) == false)
    {
        logfile.Write("ftp.chdir(%s) faild\n", starg.remotepath);
        return false;
    }

    // 注意这里 ftp.nlist(".", starg.listfilename) 第一个参数如果是"."表示只拿当前目录下的文件名，不是全路径
    // 如果这里改成 ftp.nlist(starg.remotepath, starg.listfilename) 第一个参数改为服务器目录，那么得到的结构就是全路径
    // 这里采用只有文件名比较好，第一：返回这个目录名是没有意义的，第二：去掉这些目录前缀，还可以节省网络开销（文件小）
    if(ftp.nlist(".", starg.listfilename) == false)
    {
        logfile.Write("ftp.nlist(\".\", %s) faild \n", starg.listfilename);
        return false;
    }

    // 把ftp.nlist()方法获取到的list文件加载到容器vfilelist中.
    if(LoadListFile() == false)
    {
        logfile.Write("LoadListFile() faild\n");
        return false;
    }

    // 遍历容器vfilelist

    // 在调用get方法之前，需要将文件的全路径拼接出来(分别是服务器的文件名 和 本地文件名)
    char strremotfilename[301], strlocalfilename[301];

    for(auto iter = vfilelist.begin(); iter != vfilelist.end(); ++iter)
    {
        SNPRINTF(strremotfilename, sizeof(strremotfilename), 300, "%s/%s", starg.remotepath, (*iter).filename);  // 服务器文件全路径文件名
        SNPRINTF(strlocalfilename, sizeof(strlocalfilename), 300, "%s/%s", starg.localpath, (*iter).filename);  // 本地文件全路径文件名

        // 调用ftp.get()方法从服务器中下载文件
        logfile.Write("get %s ... \n", strremotfilename);
        if(ftp.get(strremotfilename, strlocalfilename) == false)
        {
            logfile.Write("ftp.get(%s, %s) faild\n", strremotfilename, strlocalfilename);
            break;
        }

        logfile.Write("OK\n");
    }

    return true;
}

// 把ftp.nlist()方法获取到的list文件加载到vfilelist中
bool LoadListFile()
{
    vfilelist.clear();

    // 用只读的方式打开该list文件
    if(File.Open(starg.listfilename, "r") == false)
    {
        logfile.Write("File.Open(%s) faild\n", starg.listfilename);
        return false;
    }

    struct st_fileinfo stfileinfo;

    while(true)
    {
        memset(&stfileinfo, 0, sizeof(struct st_fileinfo));

        // 读取每一行，然后第三个参数设置为true，表示删除每行字符串结尾的换行和空格
        if(File.Fgets(stfileinfo.filename, 300, true) == false) break;

        // 判断文件名是否匹配我们传递进来的文件名匹配字符串，如果不匹配，说明不是我们想要的文件，就不放进容器中
        if(MatchStr(stfileinfo.filename, starg.matchname) == false) continue;

        vfilelist.push_back(stfileinfo);
    }

    return true;
}
