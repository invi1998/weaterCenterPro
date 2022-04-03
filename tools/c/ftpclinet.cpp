#include "_ftp.h"

Cftp ftp;

int main()
{
    // 采用默认的被动模式登陆
    if(ftp.login("192.168.31.166:21", "invi", "sh269jgl105") == false)
    {
        printf("登陆失败！\n");
        return -1;
    }
    printf("登陆成功！\n");



    // 获取文件时间
    if(ftp.mtime("/ftptest/cProject8_2.cpp") == false)
    {
        printf("ftp.mtime(\"/ftptest/cProject8_2.cpp\") 文件时间获取失败\n");
        ftp.logout();
        return -1;
    }

    printf("ftp.mtime(\"/ftptest/cProject8_2.cpp\") 文件时间获取成功，time = %s\n", ftp.m_mtime);

    // 、获取文件大小
    if(ftp.size("/ftptest/cProject8_2.cpp") == false)
    {
        printf("ftp.size(\"/ftptest/cProject8_2.cpp\") 文件大小获取失败\n");
        ftp.logout();
        return -1;
    }

    printf("ftp.size(\"/ftptest/cProject8_2.cpp\") 文件大小获取成功, size = %d\n", ftp.m_size);

    // 将ftp服务下的/ftptest里的子目录和文件都列举出来,并输出到 /tmp/aaa/bbb.list 中
    // 注意 nlist 只会列举出子目录，子目录中的文件不会列举出来
    if(ftp.nlist("/ftptest", "/tmp/aaa/bbb.list") == false)
    {
        printf("ftp.nlist(\"/ftptest\", \"/tmp/aaa/bbb.list\") 文件目录输出失败！\n");
        ftp.logout();
        return -1;
    }
    printf("ftp.nlist(\"/ftptest\", \"/tmp/aaa/bbb.list\") 文件和目录输出成功！\n");

    // 下载
    if(ftp.get("/ftptest/cProject8_2.cpp", "/tmp/ftptest/cProject8_2.cpp.bak", true) == false)
    {
        printf("文件下载失败\n");
        ftp.logout();
        return -1;
    }
    printf("文件下载成功\n");

    // 上传
    if(ftp.put("/project/tools/c/ftpclinet.cpp", "/ftptest/ftpclient.cpp.bak", true) == false)
    {
        printf("文件上传失败\n");
        ftp.logout();
        return -1;
    }

    printf("文件上传成功！\n");

    // 退出登陆
    ftp.logout();

    return 0;
}