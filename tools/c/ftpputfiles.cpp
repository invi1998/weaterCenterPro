#include "_ftp.h"
#include "_public.h"

CLogFile logfile;

Cftp ftp;

// 进程心跳对象
CPActive PActive;

// 程序运行参数结构体
struct st_arg
{
  char host[31];           // 远程服务器的IP和端口。
  int  mode;               // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
  char username[31];       // 远程服务器ftp的用户名。
  char password[31];       // 远程服务器ftp的密码。
  char remotepath[301];    // 远程服务器存放文件的目录。
  char localpath[301];     // 本地文件存放的目录。
  char matchname[101];     // 待上传文件匹配的规则。
  int  ptype;              // 上传后客户端文件的处理方式：1-什么也不做；2-删除；3-备份。
  char localpathbak[301]; // 上传后客户端文件的备份目录。
  char okfilename[301];    // 已上传成功文件名清单。
  int  timeout;            // 进程心跳的超时时间。
  char pname[51];          // 进程名，建议用"ftpputfiles_后缀"的方式。
} starg;

// 文件信息结构体
struct st_fileinfo
{
    char filename[301];   // 文件名。
    char mtime[21];       // 文件时间。
};

vector<struct st_fileinfo> vfilelist1;    // 已上传成功文件名的容器，从okfilename中加载。
vector<struct st_fileinfo> vfilelist2;    // 上传前列出客户端文件名的容器，从nlist文件中加载。
vector<struct st_fileinfo> vfilelist3;    // 本次不需要上传的文件的容器。
vector<struct st_fileinfo> vfilelist4;    // 本次需要上传的文件的容器。

// 处理 2， 15的处理函数
void EXIT(int sig);

// 帮助文档
void _help();

// 解析第二个参数，（解析xml得到程序运行参数），将解析结果放到starg中
bool _xmltoarg(const char* args);

// 上传文件功能的主要函数
bool _ftpputfiles();

// 把ftp.nlist()方法获取到的list文件加载到vfilelist中
bool LoadLocaltFile();

// 加载okfilename文件中的内容到容器vlistfile1中
bool LoadOKFile();

// 比较vfilelist2和vfilelist1，得到vfilelist3和vfilelist4
bool CompVector();

// 把容器vfilelist3中的内容写入到okfilename中，覆盖之前旧的okfilename文件
bool WriteToOKFile();

// 如果ptype == 1,把上传成功的文件记录追加到okfilename文件中
bool AppendToOkFile(struct st_fileinfo *stfilename);

int main(int argc, char *argv[])
{

    // 1：把服务器撒花姑娘某目录的文件全部上传到本地目录（可以指定文件名的匹配规则）
    // 日志文件名，ftp服务器的IP和端口，传输模式【主动|被动】ftp的用户名，ftp密码
    // 服务器存放文件的目录，蹦迪存放文件的目录，上传文件名匹配规则
    if(argc != 3)
    {
        _help();
        return -1;
    }

    // 把ftp服务上的某目录中的文件上传到本地目录中
    
    // 处理程序的退出信号
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
    // 但请不要用 "kill -9 +进程号" 强行终止。
    CloseIOAndSignal();
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

    // 把进程的心跳信息写入共享内存
    PActive.AddPInfo(starg.timeout, starg.pname);


    // 进入ftp服务存放文件的目录
    if(ftp.login(starg.host, starg.username, starg.password, starg.mode) == false)
    {
        logfile.Write("ftp.login(%s, %s, %s) failed\n", starg.host, starg.username, starg.password, starg.mode);
        return -1;
    }

    // 文件上传
    if(_ftpputfiles() == false)
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
    printf("Using:/project/tools/bin/ftpputfiles logfilename xmlbuffer\n\n");
                                      
    printf("Sample:/project/tools/bin/procctl 30 /project/tools/bin/ftpputfiles /log/idc/ftpputfiles_surfdata.log \"<host>127.0.0.1:21</host><mode>1</mode><username>invi</username><password>wuczpwd</password><localpath>/tmp/idc/surfdata</localpath><remotepath>/tmp/ftpputest</remotepath><matchname>SURF_ZH*.JSON</matchname><ptype>1</ptype><localpathbak>/tmp/idc/surfdatabak</localpathbak><okfilename>/idcdata/ftplist/ftpputfiles_surfdata.xml</okfilename><timeout>80</timeout><pname>ftpputfiles_surfdata</pname>\"\n\n\n");

    printf("本程序是通用的功能模块，用于把本地目录中的文件上传到远程的ftp服务器。\n");
    printf("logfilename是本程序运行的日志文件。\n");
    printf("xmlbuffer为文件上传的参数，如下：\n");
    printf("<host>127.0.0.1:21</host> 远程服务端的IP和端口。\n");
    printf("<mode>1</mode> 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。\n");
    printf("<username>wucz</username> 远程服务端ftp的用户名。\n");
    printf("<password>wuczpwd</password> 远程服务端ftp的密码。\n");
    printf("<remotepath>/tmp/ftpputest</remotepath> 远程服务端存放文件的目录。\n");
    printf("<localpath>/tmp/idc/surfdata</localpath> 本地文件存放的目录。\n");
    printf("<matchname>SURF_ZH*.JSON</matchname> 待上传文件匹配的规则。"\
            "不匹配的文件不会被上传，本字段尽可能设置精确，不建议用*匹配全部的文件。\n");
    printf("<ptype>1</ptype> 文件上传成功后，本地文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。\n");
    printf("<localpathbak>/tmp/idc/surfdatabak</localpathbak> 文件上传成功后，本地文件的备份目录，此参数只有当ptype=3时才有效。\n");
    printf("<okfilename>/idcdata/ftplist/ftpputfiles_surfdata.xml</okfilename> 已上传成功文件名清单，此参数只有当ptype=1时才有效。\n");
    printf("<timeout>80</timeout> 上传文件超时时间，单位：秒，视文件大小和网络带宽而定。\n");
    printf("<pname>ftpputfiles_surfdata</pname> 进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");

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

    GetXMLBuffer(args, "matchname", starg.matchname, 100);     // 待上传文件匹配的规则
    if(strlen(starg.matchname) == 0)
    {
        logfile.Write("matchname is null\n");
        return false;
    }

    // 上传后客户端文件的处理方式：1-什么也不做；2-删除；3-备份。
    GetXMLBuffer(args,"ptype",&starg.ptype);   
    if ( (starg.ptype!=1) && (starg.ptype!=2) && (starg.ptype!=3) )
    {
        logfile.Write("ptype is error.\n");
        return false;
    }

    GetXMLBuffer(args,"localpathbak",starg.localpathbak,300); // 上传后服务器文件的备份目录。
    if ( (starg.ptype==3) && (strlen(starg.localpathbak)==0) )
    {
        logfile.Write("localpathbak is null.\n");
        return false;
    }

    GetXMLBuffer(args,"okfilename",starg.okfilename,300); // 已上传成功文件名清单。
    if ( (starg.ptype==1) && (strlen(starg.okfilename)==0) )
    {
        logfile.Write("okfilename is null.\n");
        return false;
    }
    
    GetXMLBuffer(args,"timeout",&starg.timeout);   // 进程心跳的超时时间。
    if (starg.timeout==0)
    {
        logfile.Write("timeout is null.\n");
        return false;
    }

    GetXMLBuffer(args,"pname",starg.pname,50);     // 进程名。
    if (strlen(starg.pname)==0)
    {
        logfile.Write("pname is null.\n");
        return false;
    }

    return true;
}

bool _ftpputfiles()
{

    // 把localpath目录下的文件加载到容器vfilelist2中.
    if(LoadLocaltFile() == false)
    {
        logfile.Write("LoadLocaltFile() failed\n");
        return false;
    }

    PActive.UptATime();     // 更新进程心跳

    // 如果是增量上传
    if(starg.ptype == 1)
    {
        // 加载okfilename文件中的内容到容器vlistfile1中
        LoadOKFile();

        // 比较vfilelist2和vfilelist1，得到vfilelist3和vfilelist4
        CompVector();

        // 把容器vfilelist3中的内容写入到okfilename中，覆盖之前旧的okfilename文件
        // （这样做的目的是为了更新okfilename，不用每次都拿客户端已经有的文件作为okfilename，
        // 因为你是从服务端上传，服务端数据变了，删除了一些文件，那么你之前上传好的那些文件其实就没必要再读进来做比较了，服务端不关心这些（或者说，增量上传不关心这些））
        WriteToOKFile();

        // 把vfilelist4中的文件复制到vlistfile2中，然后就可以继续走下面的上传流程了
        vfilelist2.clear();
        vfilelist2.swap(vfilelist4);

    }

    PActive.UptATime();     // 更新进程心跳

    // 遍历容器vfilelist

    // 在调用get方法之前，需要将文件的全路径拼接出来(分别是服务器的文件名 和 本地文件名)
    char strremotfilename[301], strlocalfilename[301];

    for(auto iter = vfilelist2.begin(); iter != vfilelist2.end(); ++iter)
    {
        SNPRINTF(strremotfilename, sizeof(strremotfilename), 300, "%s/%s", starg.remotepath, (*iter).filename);  // 服务器文件全路径文件名
        SNPRINTF(strlocalfilename, sizeof(strlocalfilename), 300, "%s/%s", starg.localpath, (*iter).filename);  // 本地文件全路径文件名

        // 调用ftp.put()方法从客户端中上传文件到服务端
        logfile.Write("put %s ... ", strlocalfilename);

        // 调用ftp.put()方法把文件上传到服务器，第三个参数填true，表示上传结束需要检查文件时间来确保文件上传成功
        if(ftp.put(strlocalfilename, strremotfilename, true) == false)
        {
            logfile.WriteEx("failed\n");
            return false;
        }

        logfile.WriteEx("ok\n");

        // 每次上传完一个文件后也需要更新一下心跳
        PActive.UptATime();     // 更新进程心跳

        // 如果ptype == 1,把上传成功的文件记录追加到okfilename文件中
        if(starg.ptype == 1)
        {
            AppendToOkFile(&(*iter));
        }

        // 文件上传完成后，客户端的动作 1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。
        // 删除文件
        if(starg.ptype == 2)
        {
            if(REMOVE(strlocalfilename) == false)
            {
                logfile.Write("本地文件%s删除失败!请检查用户权限或者文件路径\n", strlocalfilename);
                return false;
            }
        }
        // 转存到备份文件
        if(starg.ptype == 3)
        {
            // 要备份文件，还需要先生成备份文件名
            char strlocalfilenamebak[301];
            SNPRINTF(strlocalfilenamebak, sizeof(strlocalfilenamebak), 300, "%s/%s", starg.localpathbak, (*iter).filename);

            // 然后调用 ftp 的rename函数给备份文件改名
            // 这里需要注意，在全部的ftp的函数中，远程的目录（strremotfilenamebak）如果不存在，是都不会创建的，只有本地目录不存在才会创建，所以需要事先在ftp服务器上创建好备份目录
            if(RENAME(strlocalfilename, strlocalfilenamebak) == false)
            {
                logfile.Write("本地文件[%s]备份（移动）到[%s]失败!请检查用户权限或者文件路径\n", strlocalfilename, strlocalfilenamebak);
                return false;
            }
        }
    }


    return true;
}

// 把localpath目录下的文件加载到容器vfilelist2中.
bool LoadLocaltFile()
{
    vfilelist2.clear();

    CDir Dir;

    Dir.SetDateFMT("yyyymmddhh24miss");

    // 不包括子目录
    // 注意：如果本地目录下的总文件数量超过10000，增量上传文件功能将会有问题
    // 建议使用deletefiles程序即使清理本地目录中的历史文件
    if(Dir.OpenDir(starg.localpath, starg.matchname) == false)
    {
       logfile.Write("Dir.OpenDir(%s, %s) failed\n", starg.localpath, starg.matchname);
       return false;
    }   

    struct st_fileinfo stfileinfo;

    while(true)
    {
        memset(&stfileinfo, 0, sizeof(struct st_fileinfo));

        if(Dir.ReadDir() == false) break;

        strcpy(stfileinfo.filename, Dir.m_FileName);        // 文件名，不包括目录名
        strcpy(stfileinfo.mtime, Dir.m_ModifyTime);         // 文件时间

        vfilelist2.push_back(stfileinfo);
    }

    return true;
}

// 加载okfilename文件中的内容到容器vlistfile1中
bool LoadOKFile()
{
    vfilelist1.clear();

    CFile File;

   if(File.Open(starg.okfilename, "r") == false)
   {
        // 注意这里，如果 okfilename 打开失败，这里是不需要放回false的，因为第一次打开这个文件，是不存在这个文件的，那么直接放回true就好了
        // vfilelist1就直接为空
        return true;
   }

   char strbuffer[501];

   struct st_fileinfo stfileinfo;

    while(true)
    {
        memset(&stfileinfo, 0, sizeof(struct st_fileinfo));

        // 读取每一行，然后第三个参数设置为true，表示删除每行字符串结尾的换行和空格
        // 将每一行xml读取到strbuffer中
        if(File.Fgets(strbuffer, 300, true) == false) break;
        // 然后将strbuffer解析到对应的结构体变量中
        GetXMLBuffer(strbuffer, "filename", stfileinfo.filename);
        GetXMLBuffer(strbuffer, "mtime", stfileinfo.mtime);

        vfilelist1.push_back(stfileinfo);
    }

    return true;
}

// 比较vfilelist2和vfilelist1，得到vfilelist3和vfilelist4
bool CompVector()
{
    vfilelist3.clear();
    vfilelist4.clear();

    auto iter1 = vfilelist2.begin();
    auto iter2 = vfilelist1.begin();

    // 遍历容器vfilelist2
    for(iter1 = vfilelist2.begin(); iter1 != vfilelist2.end(); ++iter1)
    {
        // 在容器filelist1中查找vfilelist2(iter1)的记录
        for(iter2 = vfilelist1.begin(); iter2 != vfilelist1.end(); ++iter2)
        {
            // 这里为什么不分开讨论说如果不需要比较文件时间的情况呢？
            // 因为如果不需要比较文件时间，那么文件时间都将为空，所以就算不比较文件时间的情况，这里进行文件时间比较也不会影响比较结果
            if((strcmp((*iter1).filename, (*iter2).filename) == 0) &&   // 比较文件名
                strcmp((*iter1).mtime, (*iter2).mtime) == 0)            // 比较文件时间
            {
                // 容器2中找到容器1中相同的文件名，那就把这条文件信息放入到容器3中
                vfilelist3.push_back(*iter1);
                // 这里为什么可以直接break，是因为一个容器中不会存在两个同名的文件，找到了，那么其他的就不用看了，一定是不同名的，就可以直接break这层循环了
                break;
            }
        }

        // 如果没找到，就把记录放入到vfilelist4中(也就是把vfilelist1给遍历完了还没有找到)
        if(iter2 == vfilelist1.end())
        {
            vfilelist4.push_back(*iter1);
        }
    }

    return true;
}

// 把容器vfilelist3中的内容写入到okfilename中，覆盖之前旧的okfilename文件
bool WriteToOKFile()
{
    CFile File;

    if(File.Open(starg.okfilename, "w") == false)
    {
        logfile.Write("File.Open(%s, \"w\") failed\n", starg.okfilename);
        return false;
    }

    for(auto iter = vfilelist3.begin(); iter != vfilelist3.end(); ++iter)
    {
        File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n", (*iter).filename, (*iter).mtime);
    }

    return true;
}

// 如果ptype == 1,把上传成功的文件记录追加到okfilename文件中
bool AppendToOkFile(struct st_fileinfo *stfilename)
{
    CFile File;

    if(File.Open(starg.okfilename, "a") == false)
    {
        logfile.Write("File.Open(%s, \"w\") failed\n", starg.okfilename);
        return false;
    }

    File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n", stfilename->filename, stfilename->mtime);

    return true;
}
