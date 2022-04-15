// 程序名：idcapp.cpp，此程序是数据中心项目公用函数和类的实现文件。
#include "idcapp.h"

CZHOBTMIND::CZHOBTMIND()
{
    m_conn = nullptr;
    m_logfile = nullptr;
}

CZHOBTMIND::CZHOBTMIND(connection *conn, CLogFile *logfile)
{
    m_conn = conn;
    m_logfile = logfile;
}

CZHOBTMIND::~CZHOBTMIND()
{
}

void CZHOBTMIND::BindConnLog(connection *conn, CLogFile *logfile)
{
    m_conn = conn;
    m_logfile = logfile;
}

// 把从哪文件中读取到的一行数据拆分到m_zhobtmind结构体中
bool CZHOBTMIND::SplitBuffer(char *strline, int fileType)
{
    // 初始化结构体
    memset(&m_zhobtmind, 0, sizeof(struct st_zhobtmind));

    if(fileType == 0)       // 处理xml文件格式
    {
        GetXMLBuffer(strline, "obtid", m_zhobtmind.obtid, 10);
        GetXMLBuffer(strline, "ddatetime", m_zhobtmind.ddatetime, 14);
        char tmp[11];       // 定义一个临时变量
        GetXMLBuffer(strline, "t", tmp, 10);      // xml解析出来的内容放到临时变量tmp中
        if(strlen(tmp) > 0)     // 如果取出来的临时变量不为空（表示有数据）
        {
            // 然后把它转换一下，存入结构体中去
            snprintf(m_zhobtmind.t, 10, "%d", (int)(atof(tmp)*10));
        }
        GetXMLBuffer(strline, "p", tmp, 10);      // xml解析出来的内容放到临时变量tmp中
        if(strlen(tmp) > 0)     // 如果取出来的临时变量不为空（表示有数据）
        {
            // 然后把它转换一下，存入结构体中去
            snprintf(m_zhobtmind.p, 10, "%d", (int)(atof(tmp)*10));
        }
        GetXMLBuffer(strline, "u", m_zhobtmind.u, 10);
        GetXMLBuffer(strline, "wd", m_zhobtmind.wd, 10);
        GetXMLBuffer(strline, "wf", tmp, 10); if(strlen(tmp) > 0) snprintf(m_zhobtmind.wf, 10, "%d", (int)(atof(tmp)*10));
        GetXMLBuffer(strline, "r", tmp, 10); if(strlen(tmp) > 0) snprintf(m_zhobtmind.r, 10, "%d", (int)(atof(tmp)*10));
        GetXMLBuffer(strline, "vis", tmp, 10); if(strlen(tmp) > 0) snprintf(m_zhobtmind.vis, 10, "%d", (int)(atof(tmp)*10));
    }
    else if(fileType == 1)      // 处理csv格式
    {
        CCmdStr Cmdstr;
        Cmdstr.SplitToCmd(strline, ",");

        Cmdstr.GetValue(0, m_zhobtmind.obtid, 10);
        Cmdstr.GetValue(1, m_zhobtmind.ddatetime, 14);
        char tmp[11];       // 定义一个临时变量
        Cmdstr.GetValue(2, tmp, 10);      // xml解析出来的内容放到临时变量tmp中
        if(strlen(tmp) > 0)     // 如果取出来的临时变量不为空（表示有数据）
        {
            // 然后把它转换一下，存入结构体中去
            snprintf(m_zhobtmind.t, 10, "%d", (int)(atof(tmp)*10));
        }
        Cmdstr.GetValue(3, tmp, 10);      // xml解析出来的内容放到临时变量tmp中
        if(strlen(tmp) > 0)     // 如果取出来的临时变量不为空（表示有数据）
        {
            // 然后把它转换一下，存入结构体中去
            snprintf(m_zhobtmind.p, 10, "%d", (int)(atof(tmp)*10));
        }
        Cmdstr.GetValue(4, m_zhobtmind.u, 10);
        Cmdstr.GetValue(5, m_zhobtmind.wd, 10);
        Cmdstr.GetValue(6, tmp, 10); if(strlen(tmp) > 0) snprintf(m_zhobtmind.wf, 10, "%d", (int)(atof(tmp)*10));
        Cmdstr.GetValue(7, tmp, 10); if(strlen(tmp) > 0) snprintf(m_zhobtmind.r, 10, "%d", (int)(atof(tmp)*10));
        Cmdstr.GetValue(8, tmp, 10); if(strlen(tmp) > 0) snprintf(m_zhobtmind.vis, 10, "%d", (int)(atof(tmp)*10));
    }


    STRCPY(m_buffer, sizeof(m_buffer), strline);

    return true;
}

// 把m_zhobtmind结构体中的数据插入到T_ZHOBTMIND表中
bool CZHOBTMIND::InsertTable()
{
    // 只有当stmt对象没有绑定数据库对象的时候，才进行绑定(与数据库连接的绑定状态，0-未绑定，1-已绑定)
    if(m_stmt.m_state == 0)
    {
        m_stmt.connect(m_conn);
        m_stmt.prepare("insert into T_ZHOBTMIND(obtid, ddatetime, t, p, u, wd, wf, r, vis)\
        values(:1, str_to_date(:2, '%%Y%%m%%d%%H%%i%%s'), :3, :4, :5, :6, :7, :8, :9)");
        
        // 绑定参数
        m_stmt.bindin(1, m_zhobtmind.obtid, 10);
        m_stmt.bindin(2, m_zhobtmind.ddatetime, 14);
        m_stmt.bindin(3, m_zhobtmind.t,10);
        m_stmt.bindin(4, m_zhobtmind.p,10);
        m_stmt.bindin(5, m_zhobtmind.u,10);
        m_stmt.bindin(6, m_zhobtmind.wd,10);
        m_stmt.bindin(7, m_zhobtmind.wf,10);
        m_stmt.bindin(8, m_zhobtmind.r,10);
        m_stmt.bindin(9, m_zhobtmind.vis,10);
    }

    // 把结构体中的数据插入表中
    if(m_stmt.execute() != 0)
    {
        // 失败的情况有哪些？是否全部的失败都需要写日志？
        // 失败的原因主要有二，1是记录重复，2是数据内容非法
        // 如果失败了怎么办？程序是否需要继续？是否rollback？是否返回false?
        // 如果失败的原因是内容非法，记录非法内容日志后继续，如果记录重复，不必记录日志，并且程序继续往下走
        if(m_stmt.m_cda.rc != 1062)
        {
            // 不是记录重复导致的错误
            m_logfile->Write("Buffer = %s\n", m_buffer);
            m_logfile->Write("m_stmt.execute() failed.\n%s\n%s\n", m_stmt.m_sql, m_stmt.m_cda.message);
        }
        return false;
    }
    return true;
}
