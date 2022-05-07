/*
 *  程序名：blobtofile.cpp，此程序演示开发框架操作MySQL数据库（提取BLOB字段内容到图片文件中）。
 *  author: invi
*/

#include "_mysql.h"

int main(int argc,char *argv[])
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
        long   id;             // 超女编号
        char   pic[100000];    // 超女图片的内容。(不到100k)
        unsigned long picsize; // 图片内容占用的字节数。
    } stgirls;

    sqlstatement stmt(&conn);  // 操作SQL语句的对象。

    // 准备sql语句
    stmt.prepare("select id, pic from girls where id in (1,2)");

    stmt.bindout(1, &stgirls.id);

    // 绑定BLOB字段，buffer用于存放BLOB字段的内容，buffersize为buffer占用内存的大小，
    // size为结果集中BLOB字段实际的大小，注意，一定要保证buffer足够大，防止内存溢出。
    stmt.bindoutlob(2, stgirls.pic, 100000, &stgirls.picsize);

     // 执行sql语句，一定要判断其返回值，0成功，其他是失败
    if(stmt.execute() != 0)
    {
        printf("stmt.execute() failed\n=sql=        %s\n=message=       %s\n", stmt.m_sql, stmt.m_cda.message);
        return -1;
    }

    // 获取结果集
    while (true)
    {
        memset(&stgirls, 0, sizeof(struct st_girls));

        if(stmt.next() !=0 )
        {
            break;
        }

        // 生成文件名
        char filename[10];
        memset(filename, 0, sizeof(filename));

        sprintf(filename, "%d_out.jpg", stgirls.id);

        // 把buffer中的内容写入文件filename，size为buffer中有效内容的大小。
        // 成功返回true，失败返回false。
        buftofile(filename, stgirls.pic, stgirls.picsize);
    }
    
    printf("本次查询gills表%ld条记录\n", stmt.m_cda.rpc);

    return 0;

}