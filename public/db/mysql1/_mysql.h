/**************************************************************************************/
/*   ��������_mysql.h���˳����ǿ�����ܵ�C/C++����MySQL���ݿ�������ļ���             */
/*   ���ߣ�����ܡ�                                                                   */
/**************************************************************************************/

#ifndef __MYSQL_H
#define __MYSQL_H

// C/C++�ⳣ��ͷ�ļ�
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include <mysql.h>   // MySQL���ݿ�ӿں�����ͷ�ļ�

// ���ļ�filename���ص�buffer�У�����ȷ��buffer�㹻��
// �ɹ������ļ��Ĵ�С���ļ������ڻ�Ϊ�շ���0��
unsigned long filetobuf(const char *filename,char *buffer);

// ��buffer�е�����д���ļ�filename��buffersizeΪbuffer�Ĵ�С��
// �ɹ�����true��ʧ�ܷ���false��
bool buftofile(const char *filename,char *buffer,unsigned long buffersize);

// MySQL��¼����
struct LOGINENV
{
  char ip[32];       // MySQL���ݿ��ip��ַ��
  int  port;         // MySQL���ݿ��ͨ�Ŷ˿ڡ�
  char user[32];     // ��¼MySQL���ݿ���û�����
  char pass[32];     // ��¼MySQL���ݿ�����롣
  char dbname[51];   // ��¼��ȱʡ�򿪵����ݿ⡣
};

struct CDA_DEF       // ����MySQL�ӿں���ִ�еĽ����
{
  int      rc;          // ����ֵ��0-�ɹ�������ʧ�ܡ�
  unsigned long rpc;    // �����insert��update��delete������Ӱ���¼�������������select������������������
  char     message[2048];  // ִ��SQL������ʧ�ܣ���Ŵ���������Ϣ��
};

// MySQL���ݿ������ࡣ
class connection
{
private:
  // ��connstr�н���ip,username,password,dbname,port��
  void setdbopt(char *connstr);

  // �����ַ�����Ҫ�����ݿ��һ�£��������Ļ��������
  void character(char *charset);

  LOGINENV m_env; // ���������������

  char m_dbtype[21];   // ���ݿ����࣬�̶�ȡֵΪ"mysql"��
public:
  int m_state;         // �����ݿ������״̬��0-δ���ӣ�1-�����ӡ�

  CDA_DEF m_cda;       // ���ݿ�����Ľ�������һ��ִ��SQL���Ľ����

  char m_sql[10241];   // SQL�����ı�������ܳ���10240�ֽڡ�

  connection();        // ���캯����
 ~connection();        // ����������

  // ��¼���ݿ⡣
  // connstr�����ݿ�ĵ�¼��������ʽ��"ip,username,password,dbname,port"��
  // ���磺"172.16.0.15,qxidc,qxidcpwd,qxidcdb,3306"��
  // charset�����ݿ���ַ�������"gbk"�����������ݿⱣ��һ�£�����������������������
  // autocommitopt���Ƿ������Զ��ύ��0-�����ã�1-���ã�ȱʡ�ǲ����á�
  // ����ֵ��0-�ɹ�������ʧ�ܣ�ʧ�ܵĴ�����m_cda.rc�У�ʧ�ܵ�������m_cda.message�С�
  int connecttodb(char *connstr,char *charset,unsigned int autocommitopt=0);

  // �ύ����
  // ����ֵ��0-�ɹ�������ʧ�ܣ�����Աһ�㲻�ع��ķ���ֵ��
  int commit();

  // �ع�����
  // ����ֵ��0-�ɹ�������ʧ�ܣ�����Աһ�㲻�ع��ķ���ֵ��
  int  rollback();

  // �Ͽ������ݿ�����ӡ�
  // ע�⣬�Ͽ������ݿ������ʱ��ȫ��δ�ύ�������Զ��ع���
  // ����ֵ��0-�ɹ�������ʧ�ܣ�����Աһ�㲻�ع��ķ���ֵ��
  int disconnect();

  // ִ��SQL��䡣
  // ���SQL��䲻��Ҫ�����������������ް󶨱������ǲ�ѯ��䣩������ֱ���ô˷���ִ�С�
  // ����˵��������һ���ɱ�������÷���printf������ͬ��
  // ����ֵ��0-�ɹ�������ʧ�ܣ�ʧ�ܵĴ�����m_cda.rc�У�ʧ�ܵ�������m_cda.message�У�
  // ����ɹ���ִ���˷ǲ�ѯ��䣬��m_cda.rpc�б����˱���ִ��SQLӰ���¼��������
  // ����Ա������execute�����ķ���ֵ��
  // ��connection�����ṩ��execute��������Ϊ�˷������Ա���ڸ÷����У�Ҳ����sqlstatement������ɹ��ܡ�
  int execute(const char *fmt,...);

  ////////////////////////////////////////////////////////////////////
  // ���³�Ա�����ͺ���������sqlstatement�࣬������ⲿ����Ҫ��������
  MYSQL     *m_conn;   // MySQL���ݿ����Ӿ����
  int m_autocommitopt; // �Զ��ύ��־��0-�ر��Զ��ύ��1-�����Զ��ύ��
  void err_report();   // ��ȡ������Ϣ��
  ////////////////////////////////////////////////////////////////////
};

// ִ��SQL���ǰ�����������������������ֵ��256�Ǻܴ���ˣ����Ը���ʵ�����������
#define MAXPARAMS  256

// ����SQL����ࡣ
class sqlstatement
{
private:
  MYSQL_STMT *m_handle; // SQL�������
  
  MYSQL_BIND params_in[MAXPARAMS];            // ���������
  unsigned long params_in_length[MAXPARAMS];  // ���������ʵ�ʳ��ȡ�
  my_bool params_in_is_null[MAXPARAMS];       // ��������Ƿ�Ϊ�ա�
  unsigned maxbindin;                         // ����������ı�š�

  MYSQL_BIND params_out[MAXPARAMS]; // ���������
  
  connection *m_conn;  // ���ݿ�����ָ�롣
  int m_sqltype;       // SQL�������ͣ�0-��ѯ��䣻1-�ǲ�ѯ��䡣
  int m_autocommitopt; // �Զ��ύ��־��0-�رգ�1-������
  void err_report();   // ���󱨸档
  void initial();      // ��ʼ����Ա������
public:
  int m_state;         // �����ݿ����ӵİ�״̬��0-δ�󶨣�1-�Ѱ󶨡�

  char m_sql[10241];   // SQL�����ı�������ܳ���10240�ֽڡ�

  CDA_DEF m_cda;       // ִ��SQL���Ľ����

  sqlstatement();      // ���캯����
  sqlstatement(connection *conn);    // ���캯����ͬʱ�����ݿ����ӡ�
 ~sqlstatement();      // ����������

  // �����ݿ����ӡ�
  // conn�����ݿ�����connection����ĵ�ַ��
  // ����ֵ��0-�ɹ�������ʧ�ܣ�ֻҪconn��������Ч�ģ��������ݿ���α���Դ�㹻��connect�������᷵��ʧ�ܡ�
  // ����Աһ�㲻�ع���connect�����ķ���ֵ��
  // ע�⣬ÿ��sqlstatementֻ��Ҫ��һ�Σ��ڰ��µ�connectionǰ�������ȵ���disconnect������
  int connect(connection *conn);

  // ȡ�������ݿ����ӵİ󶨡�
  // ����ֵ��0-�ɹ�������ʧ�ܣ�����Աһ�㲻�ع��ķ���ֵ��
  int disconnect();

  // ׼��SQL��䡣
  // ����˵��������һ���ɱ�������÷���printf������ͬ��
  // ����ֵ��0-�ɹ�������ʧ�ܣ�����Աһ�㲻�ع��ķ���ֵ��
  // ע�⣺���SQL���û�иı䣬ֻ��Ҫprepareһ�ξͿ����ˡ�
  int prepare(const char *fmt,...);

  // ����������ĵ�ַ��
  // position���ֶε�˳�򣬴�1��ʼ��������prepare�����е�SQL�����һһ��Ӧ��
  // value����������ĵ�ַ��������ַ������ڴ��СӦ���Ǳ���Ӧ���ֶγ��ȼ�1��
  // len�������������������������ַ�������lenָ��������󳤶ȣ�������ñ���Ӧ���ֶγ��ȡ�
  // ����ֵ��0-�ɹ�������ʧ�ܣ�����Աһ�㲻�ع��ķ���ֵ��
  // ע�⣺1�����SQL���û�иı䣬ֻ��Ҫbindinһ�ξͿ����ˣ�2��������������������ܳ���MAXPARAMS����
  int bindin(unsigned int position,int    *value);
  int bindin(unsigned int position,long   *value);
  int bindin(unsigned int position,unsigned int  *value);
  int bindin(unsigned int position,unsigned long *value);
  int bindin(unsigned int position,float *value);
  int bindin(unsigned int position,double *value);
  int bindin(unsigned int position,char   *value,unsigned int len);
  // ��BLOB�ֶΣ�bufferΪBLOB�ֶε����ݣ�sizeΪBLOB�ֶεĴ�С��
  int bindinlob(unsigned int position,void *buffer,unsigned long *size);

  // ����������ĵ�ַ��
  // position���ֶε�˳�򣬴�1��ʼ����SQL�Ľ����һһ��Ӧ��
  // value����������ĵ�ַ��������ַ������ڴ��СӦ���Ǳ���Ӧ���ֶγ��ȼ�1��
  // len�������������������������ַ�������lenָ��������󳤶ȣ�������ñ���Ӧ���ֶγ��ȡ�
  // ����ֵ��0-�ɹ�������ʧ�ܣ�����Աһ�㲻�ع��ķ���ֵ��
  // ע�⣺1�����SQL���û�иı䣬ֻ��Ҫbindoutһ�ξͿ����ˣ�2��������������������ܳ���MAXPARAMS����
  int bindout(unsigned int position,int    *value);
  int bindout(unsigned int position,long   *value);
  int bindout(unsigned int position,unsigned int  *value);
  int bindout(unsigned int position,unsigned long *value);
  int bindout(unsigned int position,float *value);
  int bindout(unsigned int position,double *value);
  int bindout(unsigned int position,char   *value,unsigned int len);
  // ��BLOB�ֶΣ�buffer���ڴ��BLOB�ֶε����ݣ�buffersizeΪbufferռ���ڴ�Ĵ�С��
  // sizeΪ�������BLOB�ֶ�ʵ�ʵĴ�С��ע�⣬һ��Ҫ��֤buffer�㹻�󣬷�ֹ�ڴ������
  int bindoutlob(unsigned int position,void *buffer,unsigned long buffersize,unsigned long *filesize);

  // ִ��SQL��䡣
  // ����ֵ��0-�ɹ�������ʧ�ܣ�ʧ�ܵĴ�����m_cda.rc�У�ʧ�ܵ�������m_cda.message�С�
  // ����ɹ���ִ���˷ǲ�ѯ��䣬��m_cda.rpc�б����˱���ִ��SQLӰ���¼��������
  // ����Ա������execute�����ķ���ֵ��
  int execute();

  // ִ��SQL��䡣
  // ���SQL��䲻��Ҫ�����������������ް󶨱������ǲ�ѯ��䣩������ֱ���ô˷���ִ�С�
  // ����˵��������һ���ɱ�������÷���printf������ͬ��
  // ����ֵ��0-�ɹ�������ʧ�ܣ�ʧ�ܵĴ�����m_cda.rc�У�ʧ�ܵ�������m_cda.message�У�
  // ����ɹ���ִ���˷ǲ�ѯ��䣬��m_cda.rpc�б����˱���ִ��SQLӰ���¼��������
  // ����Ա������execute�����ķ���ֵ��
  int execute(const char *fmt,...);

  // �ӽ�����л�ȡһ����¼��
  // ���ִ�е�SQL����ǲ�ѯ��䣬����execute�����󣬻����һ�����������������ݿ�Ļ������У���
  // next�����ӽ�����л�ȡһ����¼�����ֶε�ֵ�����Ѱ󶨵���������С�
  // ����ֵ��0-�ɹ���1403-��������޼�¼������-ʧ�ܣ�ʧ�ܵĴ�����m_cda.rc�У�ʧ�ܵ�������m_cda.message�С�
  // ����ʧ�ܵ�ԭ����Ҫ��������1�������ݿ�������ѶϿ���2��������������ڴ�̫С��
  // ÿִ��һ��next������m_cda.rpc��ֵ��1��
  // ����Ա������next�����ķ���ֵ��
  int next();
};

/*
delimiter $$
drop function if exists to_null;

create function to_null(in_value varchar(10)) returns decimal
begin
if (length(in_value)=0) then
  return null;
else
  return in_value;
end if;
end;
$$
*/

#endif

