/*
 *  程序名：crtsurfdata1.cpp  本程序用于生成全国气象站点观测的分钟数据。
 *  author：invi
*/

#include "_public.h"

// class CLogFile;

// 日志类一般会将其定义为全局的变量
CLogFile logfile(10);

int main(int argc, char *argv[])
{

  // 全国气象站点参数文件 inifile

  // 生成的测试气象数据存放的目录 outpath

  // 存放的日志 logfile


  // 所以这个程序有3个参数，那么argc的值应该为4
  if(argc!=4)
  {
    // 不等于4表示程序运行的方法不正确(这里打印提示改程序需要这3个参数)
    printf("Using:./crtsurfdata1 inifile outpath logfile\n");

    // 只提示正确方法还不够，这里给一个例子说明
    printf("Example:/project/idc1/bin/crsurfdata1 /project/idc1/ini/stcode.ini /tmp/surfdata /log/idc/crtsurfdata1.log\n\n");

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
  logfile.Write("crtsurfdata1 开始运行。\n");

  // 业务代码
  for(int i = 0; i < 10000000; i++)
  {
    logfile.Write("这是第 %d 条日志记录。", i);
  }

  logfile.Write("crtsurfdata1 运行结束。\n");

  return 0;
}