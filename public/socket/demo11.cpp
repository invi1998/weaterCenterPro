/*
 * 程序名：demo11.cpp，此程序用于演示网银app的客户端。
 * author：invi
*/
#include "../_public.h"

CTcpClient TcpClient;   // 客户端tcp通讯对象

bool srv001();    // 登陆业务
bool srv002();    // 我的账户（余额查询）
 
int main(int argc,char *argv[])
{
  if (argc!=3)
  {
    printf("Using:./demo11 ip port\nExample:./demo11 127.0.0.1 5005\n\n"); return -1;
  }


  // 向服务端发起连接请求。
  if (TcpClient.ConnectToServer(argv[1],atoi(argv[2]))==false)
  {
    printf("TcpClient.ConnectToServer(%s,%s) failed.\n",argv[1],argv[2]); return -1;
  }

  if(srv001() == false)
  {
    printf("登陆失败！\n");
    return -1;
  }

  if(srv002() == false)
  {
    printf("余额查询失败！\n");
    return -1;
  }

  return 0;

}

bool srv001()    // 登陆业务
{
  char buffer[1024];
  
  SPRINTF(buffer, sizeof(buffer), "<srvcode>1</srvcode><tel>12345678901</tel><password>123456</password>");

  printf("发送：%s。。。\n", buffer);

  if(TcpClient.Write(buffer) == false)
  {
    return false;
  }

  memset(buffer, 0, sizeof(buffer));
  if(TcpClient.Read(buffer) == false)
  {
    return false;
  }

  // 解析服务端返回的xml
  int iretcode = -1;
  GetXMLBuffer(buffer, "retcode", &iretcode);

  if(iretcode != 0)
  {
    printf("登陆失败！\n");
    return false;
  }

  printf("登陆成功！\n");

  return true;
}

bool srv002()    // 我的账户（余额查询）
{
  char buffer[1024];
  
  SPRINTF(buffer, sizeof(buffer), "<srvcode>2</srvcode><cardid>6260000000001</cardid>");

  printf("发送：%s。。。\n", buffer);

  if(TcpClient.Write(buffer) == false)
  {
    return false;
  }

  memset(buffer, 0, sizeof(buffer));
  if(TcpClient.Read(buffer) == false)
  {
    return false;
  }

  // 解析服务端返回的xml
  int iretcode = -1;
  GetXMLBuffer(buffer, "retcode", &iretcode);

  if(iretcode != 0)
  {
    printf("余额查询失败\n");
    return false;
  }

  double ye = 0;
  GetXMLBuffer(buffer, "ye", &ye);

  printf("账户余额为：%.2f \n", ye);

  return true;
}
