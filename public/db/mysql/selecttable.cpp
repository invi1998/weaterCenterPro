/*
 *  程序名：selecttable.cpp，此程序演示开发框架操作MySQL数据库（查询表中的记录）。
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

    sqlstatement stmt(&conn);

    // 准备查询表的sql语句
    /*
        注意事项：
        1、如果SQL语句的主体没有改变，只需要prepare()一次就可以了；
        2、结果集中的字段，调用bindout()绑定变量的地址；
        3、bindout()方法的返回值固定为0，不用判断返回值；
        4、如果SQL语句的主体已改变，prepare()后，需重新用bindout()绑定变量；
        5、调用execute()方法执行SQL语句，然后再循环调用next()方法获取结果集中的记录；
        6、每调用一次next()方法，从结果集中获取一条记录，字段内容保存在已绑定的变量中。
    */

   int iminid, imaxid;      // 查询条件的最小值id和最大id

    // 注意，因为btime是日期型数据，所以在select语句中需要将其转换为字符串
    stmt.prepare("\
        select id, name, weight, date_format(btime, '%%Y-%%m-%%d %%H:%%i:%%s') from girls where id >= :1 && id <= :2\
    ");

    // 把结果集的字段与变量的地址绑定。
    // position：字段的顺序，从1开始，与SQL的结果集字段一一对应。
    // value：输出变量的地址，如果是字符串，内存大小应该是表对应的字段长度加1。
    // len：如果输出变量的数据类型是字符串，用len指定它的最大长度，建议采用表对应的字段长度。
    // 返回值：0-成功，其它失败，程序中一般不必关心返回值。
    // 注意：1）如果SQL语句没有改变，只需要bindout一次就可以了，2）绑定输出变量的总数不能超过MAXPARAMS个。

    // 绑定输入(为sql语句绑定输入变量的地址，bindin不需要判断返回值)
    stmt.bindin(1, &iminid);
    stmt.bindin(2, &imaxid);

    // 绑定输出（为sql语句绑定输出变量的地址，bindout不需要判断返回值）
    stmt.bindout(1, &stgirls.id);
    stmt.bindout(2, stgirls.name, 31);
    stmt.bindout(3, &stgirls.weight);
    stmt.bindout(4, stgirls.btime, 20);

    for(int j = 0; j<5; j++)
    {
        iminid = j;
        imaxid = j + 3;

        // 执行sql语句，一定要判断返回值，0成功，其他是失败
        if(stmt.execute() != 0)
        {
            printf("stmt.execute() failed\n=sql=        %s\n=message=       %s\n", stmt.m_sql, stmt.m_cda.message);
            return -1;
        }

        // 读取girls表中的数据
        // 本程序执行的是查询语句，执行stmt.execute()之后，将会在数据库的缓冲区中产生一个结果集
        while (true)
        {
            // next() 从结果集中获取一条记录。
            // 如果执行的SQL语句是查询语句，调用execute方法后，会产生一个结果集（存放在数据库的缓冲区中）。
            // next方法从结果集中获取一条记录，把字段的值放入已绑定的输出变量中。
            // 返回值：0-成功，1403-结果集已无记录，其它-失败，失败的代码在m_cda.rc中，失败的描述在m_cda.message中。
            // 返回失败的原因主要有两种：1）与数据库的连接已断开；2）绑定输出变量的内存太小。
            // 每执行一次next方法，m_cda.rpc的值加1。
            // 程序中必须检查next方法的返回值。

            memset(&stgirls, 0, sizeof(struct st_girls));       // 结构体初始化

            // 从结果集中获取一条记录，一定要判断其返回值，0 - 成功，1403 - 无记录，其他 失败
            // 在实际开发中，除了 0 和 1403，其他情况极少出现
            if(stmt.next() != 0)
            {
                break;
            }

            // 把获取到的记录，每个字段都打印出来
            printf("id = %ld, name = %s, weight = %0.2f, btime = %s \n", stgirls.id, stgirls.name, stgirls.weight, stgirls.btime);

        }

        // 查询语句没有事务
        // 把影响条数rpc的值显示出来
        printf("本次查询影响条数： %ld \n", stmt.m_cda.rpc);
    }

   return 0;

}