#include "_tools.h"

CTABCOLS::CTABCOLS()
{
    inidata();
}

CTABCOLS::~CTABCOLS()
{
}

// 成员变量初始化
void CTABCOLS::inidata()
{
    m_allcount=m_pkcount=0;
    m_vallcols.clear();
    m_vpkcols.clear();
    memset(m_allcols, 0, sizeof(m_allcols));
    memset(m_pkcols, 0, sizeof(m_pkcols));
}

// 获取指定表的全部字段信息
bool CTABCOLS::allcols(connection *conn, char *tablename)
{
    m_allcount = 0;
    m_vallcols.clear();
    memset(m_allcols, 0, sizeof(m_allcols));

    struct st_columns stcolumns;

    sqlstatement stmt;
    stmt.connect(conn);
    stmt.prepare("select lower(column_name),lower(data_type),character_maximum_length from information_schema.COLUMNS where table_name=:1");
    stmt.bindin(1, tablename, 30);
    stmt.bindout(1, stcolumns.colname, 30);
    stmt.bindout(2, stcolumns.datatype, 30);
    stmt.bindout(3, &stcolumns.collen);

    if(stmt.execute() != 0)
    {
        return false;
    }

    while (true)
    {
        memset(&stcolumns, 0, sizeof(struct st_columns));

        if(stmt.next() != 0)
        {
            break;
        }

        // 列的数据类型，分为number，date和char 三大类
        if(strcmp(stcolumns.datatype, "char") == 0)     // 字符串类型不管是char还是varchar，都设置为char
        {
            strcpy(stcolumns.datatype, "char");
        }

        if(strcmp(stcolumns.datatype, "varchar") == 0)     // 字符串类型不管是char还是varchar，都设置为char
        {
            strcpy(stcolumns.datatype, "char");
        }

        if(strcmp(stcolumns.datatype, "datetime") == 0)     // 时间类型不管是datetime还是timestamp，都设置为date
        {
            strcpy(stcolumns.datatype, "date");
        }

        if(strcmp(stcolumns.datatype, "timestamp") == 0)     // 时间类型不管是datetime还是timestamp，都设置为date
        {
            strcpy(stcolumns.datatype, "date");
        }

        // 然后是数值类型(不管是什么数值类型，都设置为number类型)
        if(strcmp(stcolumns.datatype, "TINYINT") == 0) strcpy(stcolumns.datatype, "number");
        if(strcmp(stcolumns.datatype, "SMALLINT") == 0) strcpy(stcolumns.datatype, "number");
        if(strcmp(stcolumns.datatype, "MEDIUMINT") == 0) strcpy(stcolumns.datatype, "number");
        if(strcmp(stcolumns.datatype, "INT") == 0) strcpy(stcolumns.datatype, "number");
        if(strcmp(stcolumns.datatype, "INTEGER") == 0) strcpy(stcolumns.datatype, "number");
        if(strcmp(stcolumns.datatype, "BIGINT") == 0) strcpy(stcolumns.datatype, "number");
        if(strcmp(stcolumns.datatype, "FLOAT") == 0) strcpy(stcolumns.datatype, "number");
        if(strcmp(stcolumns.datatype, "DOUBLE") == 0) strcpy(stcolumns.datatype, "number");
        if(strcmp(stcolumns.datatype, "DECIMAL") == 0) strcpy(stcolumns.datatype, "number");
        if(strcmp(stcolumns.datatype, "numeric") == 0) strcpy(stcolumns.datatype, "number");

        // 如果业务有需要，可以修改上面的代码，增加对更多数据类型的支持

        // 如果字段的数据类型不在上面列出来的里面，就忽略它
        if( strcmp(stcolumns.datatype, "char") != 0 &&
            strcmp(stcolumns.datatype, "date") != 0 &&
            strcmp(stcolumns.datatype, "number") != 0) continue;

        // 处理字段长度，如果是日期型，长度设置为19，yyyy-mm-dd HH:mi:ss
        if(strcmp(stcolumns.datatype, "date") == 0) stcolumns.collen = 19;

        // 如果是number类型，长度设置为20
        if(strcmp(stcolumns.datatype, "number") == 0) stcolumns.collen = 20;

        // 拼接字符串的字段名
        strcat(m_allcols, stcolumns.colname);
        strcat(m_allcols, ",");     // 然后再拼接一个","进去

        // 然后把结构体放入容器中
        m_vallcols.push_back(stcolumns);

        m_allcount++;
    }
    // 在拼接字段名的时候，最后结束会多出一个“,” ，把这个逗号删除
    if(m_allcount > 0) m_allcols[strlen(m_allcols) - 1] = '\0';
    

    return true;

}

// 获取指定表的主键字段信息
bool CTABCOLS::pkcols(connection *conn, char *tablename)
{
    return true;
}
