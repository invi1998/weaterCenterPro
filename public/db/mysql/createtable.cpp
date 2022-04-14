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
  // 超女编号id，超女姓名name，体重weight，报名时间time，说明memo，图片pic
  stmt.prepare("create table girls(id      bigint(10),\
                   name    varchar(30),\
                   weight  decimal(8,2),\
                   btime   datetime,\
                   memo    longtext,\
                   pic     longblob,\
                   primary key (id))");
  // int prepare(const char* fmt, ...),sql语句可以多行书写
  // sql 语句最后的分号可有可无，建议不要写（兼容性考虑）
  // sql 语句中不能有说明文字
  // 可以不用判断stmt.prepare()的返回值，stmt.execute()时再-判断
  // 因为prepare只是给sql语句，并没有执行，所以可以等后面执行的时候再判断 execute


  // 执行sql语句，一定要判断其返回值，0 - 成功，其他失败
  // 失败代码在stmt.m_cda.rc中。失败描述在stmt.m_cda.message中
  if(stmt.execute() != 0)
  {
    // 记录错误信息，把sql语句，错误原因给打印出来。(因为在错误原因里包含了错误代码，所以这里错误代码可以不显示)
    // printf("stmt.execute() failed,\n%s\n%d\n%s\n", stmt.m_sql, stmt.m_cda.rc, stmt.m_cda.message);
    printf("stmt.execute() failed,\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message);
    return -1;
  }

  printf("create table girls ok.\n");

  return 0;
}

/*
-- 超女基本信息表。
create table girls(id      bigint(10),    -- 超女编号。
                   name    varchar(30),   -- 超女姓名。
                   weight  decimal(8,2),  -- 超女体重。
                   btime   datetime,      -- 报名时间。
                   memo    longtext,      -- 备注。
                   pic     longblob,      -- 照片。
                   primary key (id));
*/