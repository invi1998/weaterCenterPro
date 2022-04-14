/*
 *  程序名：filetoblob.cpp，此程序演示开发框架操作MySQL数据库（把图片文件存入BLOB字段）。
 *  author: invi
*/

#include "_mysql.h"

int main(int argc,char *argv[])
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
        long   id;             // 超女编号
        char   pic[100000];    // 超女图片的内容。(不到100k)
        unsigned long picsize; // 图片内容占用的字节数。
    } stgirls;

    sqlstatement stmt(&conn);  // 操作SQL语句的对象。

    // 准备sql语句
    stmt.prepare("update girls set pic = :1 where id = :2");

    // 绑定BLOB字段，buffer为BLOB字段的内容，size为BLOB字段的大小。(注意，第三个参数blob对象的大小，填的是size的地址)
    stmt.bindinlob(1, stgirls.pic, &stgirls.picsize);
    stmt.bindin(2, &stgirls.id);

    // 修改girls表中id = 1,2的记录，并把图片存进去
    for(int i = 0; i < 2; i++)
    {
        memset(&stgirls, 0, sizeof(struct st_girls));
        stgirls.id = i +1;

        // 把图片内容加载到pic成员中
        // _mysql.h中提供了filetobuffer这个函数
        // 把文件filename加载到buffer中，必须确保buffer足够大。
        // 成功返回文件的大小，文件不存在或为空返回0。
        if(i == 0)
        {
            stgirls.picsize = filetobuf("1.jpg", stgirls.pic);
        }

        if(i == 1)
        {
            stgirls.picsize = filetobuf("2.jpg", stgirls.pic);
        }

        // 执行sql语句，一定要判断其返回值，0成功，其他是失败
        if(stmt.execute() != 0)
        {
            printf("stmt.execute() failed\n=sql=        %s\n=message=       %s\n", stmt.m_sql, stmt.m_cda.message);
            return -1;
        }

        printf("成功修改了%ld条记录\n", stmt.m_cda.rpc);
    }

    printf("update ok\n");

    // 数据更新会产生事务，所以需要提交事务
    conn.commit();

  return 0;

}