/*
 *  ��������blobtofile.cpp���˳�����ʾ������ܲ���MySQL���ݿ⣨��ȡBLOB�ֶ����ݵ�ͼƬ�ļ��У���
 *  ���ߣ�����ܡ�
*/
#include "_mysql.h"   // ������ܲ���MySQL��ͷ�ļ���

// �������ڳ�Ů��Ϣ�Ľṹ������е��ֶζ�Ӧ��
struct st_girls
{
  long id;               // ��Ů��ţ���long�������Ͷ�ӦOracle��С����number(10)��
  char name[31];         // ��Ů��������char[31]��ӦOracle��varchar2(30)��
  double weight;         // ��Ů���أ���double�������Ͷ�ӦOracle��С����number(8,2)��
  char btime[20];        // ����ʱ�䣬��char��ӦOracle��date����ʽ��'yyyy-mm-dd hh24:mi:ss'��
  char pic[100000];      // ��ŮͼƬ��
  unsigned long picsize; // ͼƬ��С��
} stgirls;

int main(int argc,char *argv[])
{
  connection conn; // ���ݿ�������
  
  // ��¼���ݿ⣬����ֵ��0-�ɹ�������-ʧ�ܡ�
  // ʧ�ܴ�����conn.m_cda.rc�У�ʧ��������conn.m_cda.message�С�
  if (conn.connecttodb("127.0.0.1,root,mysqlpwd,mysql,3306","gbk") != 0)
  {
    printf("connect database failed ghfgh.\n%s\n",conn.m_cda.message); return -1;
  }

  sqlstatement stmt(&conn); // ����SQL���Ķ���

  // ׼����ѯ����SQL��䡣
  stmt.prepare("select pic from girls ");
  stmt.bindoutlob(1,stgirls.pic,100000,&stgirls.picsize);

  // ִ��SQL��䣬һ��Ҫ�жϷ���ֵ��0-�ɹ�������-ʧ�ܡ�
  // ʧ�ܴ�����stmt.m_cda.rc�У�ʧ��������stmt.m_cda.message�С�
  if (stmt.execute() != 0)
  {
    printf("stmt.execute() failed.\n%s\n%s\n",stmt.m_sql,stmt.m_cda.message); return -1;
  }

  // ������ִ�е��ǲ�ѯ��䣬ִ��stmt.execute()�󣬽��������ݿ�Ļ������в���һ���������
  while (1)
  {
    memset(&stgirls,0,sizeof(stgirls)); // �Ȱѽṹ�������ʼ����

    // �ӽ�����л�ȡһ����¼��һ��Ҫ�жϷ���ֵ��0-�ɹ���1403-�޼�¼������-ʧ�ܡ�
    // ��ʵ�ʿ����У�����0��1403��������������ٳ��֡�
    if (stmt.next() !=0) break;
    
    // �ѻ�ȡ�������ݵĴ�С��ӡ������
    printf("size=%ld\n",stgirls.picsize);

    buftofile("/tmp/girl.jpeg",stgirls.pic,stgirls.picsize);
  }

  // ��ע�⣬stmt.m_cda.rpc�����ǳ���Ҫ����������SQL��ִ�к�Ӱ��ļ�¼����
  printf("���β�ѯ��girls��%ld����¼��\n",stmt.m_cda.rpc);
}

