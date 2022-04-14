/*
 *  程序名：inserttable.cpp，此程序演示开发框架操作MySQL数据库（向表中插入5条记录）。
 *  author: invi
*/

#include "_mysql.h"

int main(int argc, char * argv[])
{
    connection conn;   // 数据库连接类。

    // 登录数据库，返回值：0-成功；其它是失败，存放了MySQL的错误代码。
    // 失败代码在conn.m_cda.rc中，失败描述在conn.m_cda.message中。
    if(conn.connecttodb("192.168.31.133,root,sh269jgl105,mysql,3306", "utf8") != 0)
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

    sqlstatement stmt(&conn);  // 操作SQL语句的对象。

    // 准备插入表的sql语句(因为这里我们插入的时间是字符串，所以这里需要做一个字符串转时间，同时注意，因为%在c语音中是有特殊含义的，
    // 所以在str_to_date里的格式化字符串占位符%需要做转义，故这里会有%%)
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
        insert into girls (id, name, weight, btime) values(:1, :2, :3, str_to_date(:4, '%%Y-%%m-%%d %%H:%%i:%%s'))\
    ");

    // 也可以使用 ? 来代替 :1...这些占位符
    // stmt.prepare("\
    //     insert into girls (id, name, weight, btime) values(?, ?, ?, str_to_date(?, '%%Y-%%m-%%d %%H:%%i:%%s'))\
    // ");

    // 参数可以参与运算或用于函数的参数；
    // stmt.prepare("\
    //     insert into girls (id, name, weight, btime) values(:1+5, :2, :3 + 2.11, str_to_date(:4, '%%Y-%%m-%%d %%H:%%i:%%s'))\
    // ");

    // 绑定输入变量的地址。
    // position：字段的顺序，从1开始，必须与prepare方法中的SQL的序号一一对应。
    // value：输入变量的地址，如果是字符串，内存大小应该是表对应的字段长度加1。
    // len：如果输入变量的数据类型是字符串，用len指定它的最大长度，建议采用表对应的字段长度。
    // 返回值：0-成功，其它失败，程序中一般不必关心返回值。
    // 注意：1）如果SQL语句没有改变，只需要bindin一次就可以了，2）绑定输入变量的总数不能超过MAXPARAMS个。
    stmt.bindin(1, &stgirls.id);
    stmt.bindin(2, stgirls.name, 30);
    stmt.bindin(3, &stgirls.weight);
    stmt.bindin(4, stgirls.btime, 19);

    // 模拟超女数据，向表中插入5条记录
    for(int i = 0; i < 5; i++)
    {
        memset(&stgirls, 0, sizeof(struct st_girls));       // 结构体变量初始化

        // 为结构体变量的成员赋值
        stgirls.id = i;                                 // 超女编号
        sprintf(stgirls.name, "超女%05dgirl", i + 1);      // 超女姓名
        stgirls.weight = 45.33 + i;                         // 超女体重
        sprintf(stgirls.btime, "2022-02-%02d 10:12:%02d", i+1, i+2);    // 报名时间

        if(stmt.execute()!=0)
        {
            // 如果执行失败，记录日志并打印
            printf("stmt.execute() failed\n=sql= %s\n=message= %s\n", stmt.m_sql, stmt.m_cda.message);
            return -1;
        }

        // 然后注意，如果是增删改，那么执行成功后，会将影响的行数记录在rpc这个成员变量中
        // 这里如果插入成功，记录一下数据库收影响的函数
        printf("成功插入了%ld行记录\n", stmt.m_cda.rpc);    // stmt.m_cda.rpc是本次执行sql影响的记录数
    }

    printf("insert table girls ok\n");

    // 事务提交
    conn.commit();

    return 0;
}