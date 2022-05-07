/*
 *  程序名：updatetable.cpp，此程序演示开发框架操作MySQL数据库（修改表中的记录）。
 *  author: invi
*/

#include "_mysql.h"

int main(int argc, char * argv[])
{
    connection conn;   // 数据库连接类。

    // 登录数据库，返回值：0-成功；其它是失败，存放了MySQL的错误代码。
    // 失败代码在conn.m_cda.rc中，失败描述在conn.m_cda.message中。
    if(conn.connecttodb("192.168.31.133,root,19981021115,mysql,3306", "utf8") != 0)
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

    // 准备更新表的sql语句
    /*
        注意事项：
        1、参数的序号从1开始，连续、递增，参数也可以用问号表示，但是，问号的兼容性不好，不建议；
        2、SQL语句中的右值才能作为参数，表名、字段名、关键字、函数名等都不能作为参数；
        3、参数可以参与运算或用于函数的参数；
        4、如果SQL语句的主体没有改变，只需要prepare()一次就可以了；
        5、SQL语句中的每个参数，必须调用bindin()绑定变量的地址；
        6、如果SQL语句的主体已改变，prepare()后，需重新用bindin()绑定变量；
        7、prepare()方法有返回值，一般不检查，如果SQL语句有问题，调用execute()方法时能发现；
        8、bindin()方法的返回值固定为0，不用判断返回值；
        9、prepare()和bindin()之后，每调用一次execute()，就执行一次SQL语句，SQL语句的数据来自被绑定变量的值。
    */
    stmt.prepare("\
        update girls set name=:1,weight=:2,btime=str_to_date(:3,'%%Y-%%m-%%d %%H:%%i:%%s') where id = :4\
    ");

    // 使用bindin绑定参数
    stmt.bindin(1, stgirls.name, 30);
    stmt.bindin(2, &stgirls.weight);
    stmt.bindin(3, stgirls.btime, 19);
    stmt.bindin(4, &stgirls.id);

    // 将girls表中前5条记录进行更新
    for(int i = 0; i < 5; i++)
    {
        memset(&stgirls, 0, sizeof(struct st_girls));

        // 为结构体成员赋值
        stgirls.id = i;
        sprintf(stgirls.name, "貂蝉%05dchina", i + 1);
        stgirls.weight = 41.23 + i;
        sprintf(stgirls.btime, "2022-01-%02d %02d:%02d:%02d", (i+1)*2, i+2, i+1, i*3);


        // 执行sql语句，一定要判断返回值 0 =成功 ，其他 = 失败
        // 失败的代码在 stmt.m_cda.rc中。失败的描述在stmt.m_cda.message中
        if(stmt.execute() != 0)
        {
            printf("stmt.execute() failed\n=sql=        %s\n=message=       %s\n", stmt.m_sql, stmt.m_cda.message);
            return -1;
        }

        printf("成功修改了%ld条记录\n", stmt.m_cda.rpc);        // stmt.m_cda.rpc中是本次执行sql影响的记录数
    }

    printf("update table girls ok.\n");

    conn.commit();      // 事务提交

    return 0;

}