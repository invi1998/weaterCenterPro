#include "_ftp.h"
#include "_public.h"

CLogFile logfile;

Cftp ftp;

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
  int  ptype;              // 下载后服务器文件的处理方式：1-什么也不做；2-删除；3-备份。
  char remotepathbak[301]; // 下载后服务器文件的备份目录。
  char okfilename[301];    // 已下载成功文件名清单。
  bool checkmtime;         // 是否需要检查服务端文件的时间，true-需要，false-不需要，缺省为false。
} starg;

// 文件信息结构体
struct st_fileinfo
{
    char filename[301];   // 文件名。
    char mtime[21];       // 文件时间。
};

vector<struct st_fileinfo> vfilelist1;    // 已下载成功文件名的容器，从okfilename中加载。
vector<struct st_fileinfo> vfilelist2;    // 下载前列出服务器文件名的容器，从nlist文件中加载。
vector<struct st_fileinfo> vfilelist3;    // 本次不需要下载的文件的容器。
vector<struct st_fileinfo> vfilelist4;    // 本次需要下载的文件的容器。

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

// 加载okfilename文件中的内容到容器vlistfile1中
bool LoadOKFile();

// 比较vfilelist2和vfilelist1，得到vfilelist3和vfilelist4
bool CompVector();

// 把容器vfilelist3中的内容写入到okfilename中，覆盖之前旧的okfilename文件
bool WriteToOKFile();

// 如果ptype == 1,把下载成功的文件记录追加到okfilename文件中
bool AppendToOkFile(struct st_fileinfo *stfilename);

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

    printf("Sample:/project/tools/bin/procctl 30 /project/tools/bin/ftpgetfiles /log/idc/ftpgetfiles_surfdata.log \"<host>127.0.0.1:21</host><mode>1</mode><username>wucz</username><password>wuczpwd</password><localpath>/idcdata/surfdata</localpath><remotepath>/tmp/idc/surfdata</remotepath><matchname>SURF_ZH*.XML,SURF_ZH*.CSV</matchname><listfilename>/idcdata/ftplist/ftpgetfiles_surfdata.list</listfilename><ptype>1</ptype><remotepathbak>/tmp/idc/surfdatabak</remotepathbak><okfilename>/idcdata/ftplist/ftpgetfiles_surfdata.xml</okfilename><checkmtime>true</checkmtime>\"\n\n\n");

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
    printf("<listfilename>/idcdata/ftplist/ftpgetfiles_surfdata.list</listfilename> 下载前列出服务器文件名的文件。\n");
    printf("<ptype>1</ptype> 文件下载成功后，远程服务器文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。\n");
    printf("<remotepathbak>/tmp/idc/surfdatabak</remotepathbak> 文件下载成功后，服务器文件的备份目录，此参数只有当ptype=3时才有效。\n");
    printf("<okfilename>/idcdata/ftplist/ftpgetfiles_surfdata.xml</okfilename> 已下载成功文件名清单，此参数只有当ptype=1时才有效。\n");
    printf("<checkmtime>true</checkmtime> 是否需要检查服务端文件的时间，true-需要，false-不需要，此参数只有当ptype=1时才有效，缺省为false。\n\n\n");
  
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

    // 下载后服务器文件的处理方式：1-什么也不做；2-删除；3-备份。
    GetXMLBuffer(args,"ptype",&starg.ptype);   
    if ( (starg.ptype!=1) && (starg.ptype!=2) && (starg.ptype!=3) )
    {
        logfile.Write("ptype is error.\n");
        return false;
    }

    GetXMLBuffer(args,"remotepathbak",starg.remotepathbak,300); // 下载后服务器文件的备份目录。
    if ( (starg.ptype==3) && (strlen(starg.remotepathbak)==0) )
    {
        logfile.Write("remotepathbak is null.\n");
        return false;
    }

    GetXMLBuffer(args,"okfilename",starg.okfilename,300); // 已下载成功文件名清单。
    if ( (starg.ptype==1) && (strlen(starg.okfilename)==0) )
    {
        logfile.Write("okfilename is null.\n");
        return false;
    }

    // 是否需要检查服务端文件的时间，true-需要，false-不需要，此参数只有当ptype=1时才有效，缺省为false。
    GetXMLBuffer(args,"checkmtime",&starg.checkmtime);

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

    // 把ftp.nlist()方法获取到的list文件加载到容器vfilelist2中.
    if(LoadListFile() == false)
    {
        logfile.Write("LoadListFile() faild\n");
        return false;
    }

    // 如果是增量下载
    if(starg.ptype == 1)
    {
        // 加载okfilename文件中的内容到容器vlistfile1中
        LoadOKFile();

        // 比较vfilelist2和vfilelist1，得到vfilelist3和vfilelist4
        CompVector();

        // 把容器vfilelist3中的内容写入到okfilename中，覆盖之前旧的okfilename文件
        // （这样做的目的是为了更新okfilename，不用每次都拿客户端已经有的文件作为okfilename，
        // 因为你是从服务端下载，服务端数据变了，删除了一些文件，那么你之前下载好的那些文件其实就没必要再读进来做比较了，服务端不关心这些（或者说，增量下载不关心这些））
        WriteToOKFile();

        // 把vfilelist4中的文件复制到vlistfile2中，然后就可以继续走下面的下载流程了
        vfilelist2.clear();
        vfilelist2.swap(vfilelist4);

    }

    // 遍历容器vfilelist

    // 在调用get方法之前，需要将文件的全路径拼接出来(分别是服务器的文件名 和 本地文件名)
    char strremotfilename[301], strlocalfilename[301];

    for(auto iter = vfilelist2.begin(); iter != vfilelist2.end(); ++iter)
    {
        SNPRINTF(strremotfilename, sizeof(strremotfilename), 300, "%s/%s", starg.remotepath, (*iter).filename);  // 服务器文件全路径文件名
        SNPRINTF(strlocalfilename, sizeof(strlocalfilename), 300, "%s/%s", starg.localpath, (*iter).filename);  // 本地文件全路径文件名

        // 调用ftp.get()方法从服务器中下载文件
        logfile.Write("get %s ... \n", strremotfilename);
        if(ftp.get(strremotfilename, strlocalfilename) == false)
        {
            logfile.Write("ftp.get(%s, %s) faild\n", strremotfilename, strlocalfilename);
            return false;
        }

        logfile.Write("OK\n");

        // 如果ptype == 1,把下载成功的文件记录追加到okfilename文件中
        if(starg.ptype == 1)
        {
            AppendToOkFile(&(*iter));
        }

        // 文件下载完成后，服务端的动作 1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。
        // 删除文件
        if(starg.ptype == 2)
        {
            if(ftp.ftpdelete(strremotfilename) == false)
            {
                logfile.Write("文件%s删除失败!请检查用户权限或者文件路径\n", strremotfilename);
                return false;
            }
        }
        // 转存到备份文件
        if(starg.ptype == 3)
        {
            // 要备份文件，还需要先生成备份文件名
            char strremotfilenamebak[301];
            SNPRINTF(strremotfilenamebak, sizeof(strremotfilenamebak), 300, "%s/%s", starg.remotepathbak, (*iter).filename);

            // 然后调用 ftp 的rename函数给备份文件改名
            // 这里需要注意，在全部的ftp的函数中，远程的目录（strremotfilenamebak）如果不存在，是都不会创建的，只有本地目录不存在才会创建，所以需要事先在ftp服务器上创建好备份目录
            if(ftp.ftprename(strremotfilename, strremotfilenamebak) == false)
            {
                logfile.Write("文件[%s]备份（移动）到[%s]失败!请检查用户权限或者文件路径\n", strremotfilename, strremotfilenamebak);
                return false;
            }
        }
    }


    return true;
}

// 把ftp.nlist()方法获取到的list文件加载到vfilelist容器中
bool LoadListFile()
{
    CFile File;
    vfilelist2.clear();

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

        // 当文件下载模式为增量下载，并且需要进行文件时间校验（校验是否有文件被更改），就需要将文件时间读进结构体中
        if(starg.ptype == 1 && starg.checkmtime == true)
        {
            // 获取服务端文件的时间
            if(ftp.mtime(stfileinfo.filename) == false)
            {
                logfile.Write("ftp.mtime(%s) faild \n", stfileinfo.filename);
                return false;
            }
            // 将文件时间拷贝进mtime中
            strcpy(stfileinfo.mtime, ftp.m_mtime);
        }

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
        logfile.Write("File.Open(%s, \"w\") faild\n", starg.okfilename);
        return false;
    }

    for(auto iter = vfilelist3.begin(); iter != vfilelist3.end(); ++iter)
    {
        File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n", (*iter).filename, (*iter).mtime);
    }

    return true;
}

// 如果ptype == 1,把下载成功的文件记录追加到okfilename文件中
bool AppendToOkFile(struct st_fileinfo *stfilename)
{
    CFile File;

    if(File.Open(starg.okfilename, "a") == false)
    {
        logfile.Write("File.Open(%s, \"w\") faild\n", starg.okfilename);
        return false;
    }

    File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n", stfilename->filename, stfilename->mtime);

    return true;
}
