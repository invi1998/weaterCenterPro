/*
 *  程序名：createtable.cpp，此程序演示开发框架操作MySQL数据库（创建表）
 *  author：invi
*/

#include "_mysql.h"       // 开发框架操作MySQL的头文件。

int main(int argc, char* argv[])
{
  connection conn;      // 数据库连接类

  // 登陆数据库
  if(conn.connecttodb("192.168.31.133,root,sh269jgl105,mysql,3306", "utf8") != 0)
  {
    // 登陆数据库，肯定要判断返回值，如果失败了，需要将错误信息做一个提示（如果是正式服务，需要记录日志）
    printf("connect database failed\n %s\n", conn.m_cda.message);
    return -1;
  }

  // 操作sql语句的对象（创建该对象有两种方式创建）
  sqlstatement stmt(&conn);
  // 或者
  // sqlstatement stmt;
  // stmt.connect(&conn);

  // 准备创建超级女生信息表
  // 超女编号

  return 0;
}