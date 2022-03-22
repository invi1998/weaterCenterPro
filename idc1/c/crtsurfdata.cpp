/*
 *  程序名：crtsurfdata.cpp  本程序用于生成全国气象站点观测的分钟数据。
 *  author：invi
*/

#include "_public.h"

// class CLogFile;

// 日志类一般会将其定义为全局的变量
CLogFile logfile;

// 气象站点参数数据eg（结构体）
// 省   站号  站名 纬度   经度  海拔高度
// 安徽,58015,砀山,34.27,116.2,44.2
struct st_stcode
{
  char provname[31];      // 省代码（省名）
  char obtid[11];         // 站号
  char obtname[31];       // 站名
  double lat;             // 纬度
  double lon;             // 经度
  double height;          // 海拔高度
};

// 定义一个存放站点参数的容器
vector<st_stcode> vstcode;

// 把气象站点参数加载到参数容器中的函数
bool LoadSTCode(const char * inifile);

int main(int argc, char *argv[])
{

  // 全国气象站点参数文件 inifile

  // 生成的测试气象数据存放的目录 outpath

  // 存放的日志 logfile


  // 所以这个程序有3个参数，那么argc的值应该为4
  if(argc!=4)
  {
    // 不等于4表示程序运行的方法不正确(这里打印提示改程序需要这3个参数)
    printf("Using:./crtsurfdata inifile outpath logfile\n");

    // 只提示正确方法还不够，这里给一个例子说明
    printf("Example:/project/idc1/bin/crtsurfdata /project/idc1/ini/stcode.ini /tmp/surfdata /log/idc/crtsurfdata.log\n\n");

    // 然后对这些参数做一个详细的打印说明
    printf("全国气象站点参数文件 : inifile \n");
    printf("生成的测试气象数据存放的目录 : outpath\n");
    printf("存放的日志 : logfile\n\n");

    // 程序退出
    return -1;
  }

  // 打开日志
  if(logfile.Open(argv[3]) == false)
  {
    // 日志文件打开失败
    printf("logfile.Open(%s) faild.\n", argv[3]);
    // 程序退出
    return -1;
  }

  // 往日志中写入内容
  logfile.Write("crtsurfdata 开始运行。\n");

  // 业务代码
  // 把气象站点参数加载到参数容器中的函数
  if(LoadSTCode(argv[1]) == false)
  {
    return -1;
  }


  logfile.Write("crtsurfdata 运行结束。\n");

  return 0;
}

// 把气象站点参数加载到参数容器中的函数
bool LoadSTCode(const char * inifile)
{
  CFile File;
  // 打开站点参数文件(用只读的方式打开文件)
  if(File.Open(inifile, "r") == false)
  {
    // 文件打开失败，写日志，函数返回
    logfile.Write("File.Open(%s) faild.\n", inifile);
    return false;
  }

  // 定义一个buffer，用于存放从文件中读取到的每一行数据
  char strBuffer[301];

  // 用于字符串拆分的类
  CCmdStr CmdStr;

  // 定义用于保存拆分后的结构体
  struct st_stcode stcode;

  while(true)
  {
    // 从站点参数文件中读取一行，如果已经读取完毕，跳出循环
    // 字符串变量在每一次使用之前，最好进行初始化，但是我们的fgets这个成员函数中，有对buffer的初始化，所以这里可以不写初始化代码
    // memset(strBuffer, 0, sizeof(strBuffer));
    if(File.Fgets(strBuffer, 300, true) == false)
    {
      break;
    }

    // 这行代码主要是为了处理站点数据文件中第一行的无效数据（因为这行数据没有用","分割，所以拆分结果是1，就不会是6，所以这行拆分数据就给忽略就好
    if(CmdStr.CmdCount() != 6) continue;

    // 把读取到的每一行进行拆分, 拆分出来的数据保存到strBuffer中，按","进行拆分，然后删除拆分出来的字符串的前后的空格
    CmdStr.SplitToCmd(strBuffer, ",", true);

    // 把站点参数的每一个数据项保存到站点参数结构体中
    CmdStr.GetValue(0, stcode.provname, 30);    // 省代码（省名
    CmdStr.GetValue(1, stcode.obtid, 10);       // 站号
    CmdStr.GetValue(2, stcode.obtname, 30);     // 站名
    CmdStr.GetValue(3, &stcode.lat);            // 纬度
    CmdStr.GetValue(4, &stcode.lon);            // 经度
    CmdStr.GetValue(5, &stcode.height);         // 海拔高度

    // 把站点参数结构体放入站点参数容器中
    vstcode.push_back(stcode);

  }

  // 关闭文件(这里不用关闭，在CFile类的析构函数中有文件关闭代码，因为是栈上定义的变量，所以超出作用域会自动释放)

  return true;
}