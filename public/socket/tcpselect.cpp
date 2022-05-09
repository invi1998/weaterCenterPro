#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>

// 初始化服务端的监听端口
int initserver(int port);

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("usage: ./tcpselect port\n");
        return -1;
    }

    // 初始化服务端用于监听的socket
    int listensock = initserver(atoi(argv[1]));

    printf("listensock = %d\n", listensock);

    if(listensock < 0)
    {
        printf("初始化监听端口失败..\n");
        return -1;
    }

    fd_set readfds;         // 声明一个socket集合，包括socket和客户端连接上来的socket
    FD_ZERO(&readfds);      // 初始化读事件socket的集合
    FD_SET(listensock, &readfds);       // 把监听的socket加入到这个socket集合中

    int maxfd = listensock;             // 记录集合中socket的最大值

    while (true)
    {
        // 事件：1)新客户端的连接请求accept；2)客户端有报文到达recv，可以读；3)客户端连接已断开；
        //       4)可以向客户端发送报文send，可以写。
        // 可读事件  可写事件
        // select() 等待事件的发生(监视哪些socket发生了事件)。

        struct timeval timeout;
        timeout.tv_sec=10;          // 10s
        timeout.tv_usec=0;          // 0微秒

        fd_set tmpfds = readfds;
        int infds = select(maxfd+1, &tmpfds, NULL, NULL, &timeout);     // 这里把socket集合的副本传给select

        // 返回失败
        if(infds < 0)
        {
            perror("select() failed\n");
            break;
        }

        // 超时，在本程序中，select函数最后一个参数为空，不存在超时的情况，单已下代码还是留着‘
        if(infds == 0)
        {
            printf("select timeout\n");
            continue;
        }

        // 如果infds > 0.表示有事件发生的socket的数量
        for(int eventfd = 0; eventfd <= maxfd; eventfd++)
        {
            // 使用FD_ISSET来检测socket集合中是否有事件
            if(FD_ISSET(eventfd, &tmpfds) <= 0)
            {
                continue;       // 没有事件，继续循环
            }

            // 接下来就是表示找到了发生事件的socket

            // 如果发生的事件是listensock。表示有新的客户端连接上来
            // 监听socket只会用于监听客户端的连接请求，不会接收客户端的数据通信报文
            if(eventfd == listensock)
            {
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int clientsock = accept(listensock, (struct sockaddr*)&client, &len);
                if(clientsock < 0)
                {
                    perror("accept() failed\n");
                    continue;
                }

                printf("accept client(socket=%d) ok\n", clientsock);

                // 然后把新客户端的socket加入可读socket的集合
                FD_SET(clientsock, &readfds);
                if(maxfd < clientsock) 
                {
                    maxfd = clientsock;     // 更新maxfd的值
                }
            }
            else
            {
                // 如果是客户端连接的socket有事件，表示有报文发送过来，或者连接已经断开
                char buffer[1024];      // 存放从客户端读取的数据
                memset(buffer, 0, sizeof(buffer));

                if(recv(eventfd, buffer, sizeof(buffer), 0) <= 0)
                {
                    // 客户端连接已经断开
                    printf("client(enventfd = %d) disconnected\n", eventfd);
                    close(eventfd);
                    // 然后用这个宏把已经关闭的socket从socket集合中删除
                    FD_CLR(eventfd, &readfds);
                    // 从集合中删除一个socket之后，要重新计算maxfd的值
                    if(maxfd == eventfd)
                    {
                        for(int i = maxfd; i > 0; i--)      // 从后往前找
                        {
                            if(FD_ISSET(i, &readfds))
                            {
                                maxfd = i;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    // 客户端有报文发送过来
                    printf("recv(eventfd=%d):%s\n", eventfd, buffer);
                    // 这里把接收到的报文原封不动的发回去
                    fd_set tmpfds;
                    FD_ZERO(&tmpfds);
                    FD_SET(eventfd, &tmpfds);
                    if(select(eventfd+1, NULL, &tmpfds, NULL, NULL) <= 0)
                    {
                        perror("select（） failed");
                    }
                    else
                    {
                        send(eventfd, buffer, strlen(buffer), 0);
                    }
                }
            }
        }
    }
    

    return 0;
}

// 初始化服务端的监听端口
int initserver(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0)
    {
        perror("socket() failed");
        return -1;
    }

    int opt = 1;
    unsigned int len = sizeof(opt);

    struct sockaddr_in servadr;
    servadr.sin_family = AF_INET;
    servadr.sin_addr.s_addr = htonl(INADDR_ANY);
    servadr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&servadr, sizeof(servadr)) < 0)
    {
        perror("bind() faied");
        close(sock);
        return -1;
    }

    if(listen(sock, 5) != 0)
    {
        perror("listen() failed");
        close(sock);
        return -1;
    }
    
    return sock;
}