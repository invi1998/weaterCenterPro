/*
 *  程序名：deletetable.cpp，此程序演示开发框架操作MySQL数据库（删除表中的记录）。
 *  author: invi
*/

#include "_mysql.h"

int main(int argc, char * argv[])
{
    connection conn;   // 数据库连接类。

    // 登录数据库，返回值：0-成功；其它是失败，存放了MySQL的错误代码。
    // 失败代码在conn.m_cda.rc中，失败描述在conn.m_cda.message中。
    if(conn.connecttodb("192.168.31.133,root,password,mysql,3306", "utf8") != 0)
    {
        // 登陆数据库，肯定要判断返回值，如果失败了，需要将错误信息做一个提示（如果是正式服务，需要记录日志）
        printf("connect database failed\n %s\n", conn.m_cda.message);
        return -1;
    }

    // 定义用于超女信息的结构，与表中的字段对应。
    struct st_girls
    {
        long   id;        // 超女编号
        char   name[31];  // 超女姓名
        double weight;    // 超女体重
        char   btime[20]; // 报名时间
    } stgirls;

    sqlstatement stmt(&conn);

    int iminid, imaxid;      // 删除条件的最小值id和最大id

    // 准备输出sql语句
    stmt.prepare("\
        delete from girls where id >= :1 && id <= :2\
    ");

    // 绑定输入(为sql语句绑定输入变量的地址，bindin不需要判断返回值)
    stmt.bindin(1, &iminid);
    stmt.bindin(2, &imaxid);

    imaxid = 2;
    iminid = 0;

    // 执行sql语句，一定要判断返回值，0成功，其他是失败
    if(stmt.execute() != 0)
    {
        printf("stmt.execute() failed\n=sql=        %s\n=message=       %s\n", stmt.m_sql, stmt.m_cda.message);
        return -1;
    }

    printf("删除成功，删除了girls表中%ld条记录\n", stmt.m_cda.rpc);

    // 删除表中的数据会产生事务，所以这里需要提交事务
    conn.commit();

    return 0;

}