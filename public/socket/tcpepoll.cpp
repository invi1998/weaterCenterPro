#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>

// 初始化服务端的监听端口
int initserver(int port);

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("usage: ./tcpepoll port\n");
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

    // 创建epoll句柄
    int epollfd = epoll_create(1);

    // 为监听socket准备可读事件
    epoll_event ev;             // 声明事件的数据结构
    ev.events = EPOLLIN;        // 对结构体的events成员赋值，表示关心读事件
    ev.data.fd = listensock;    // 对结构体的用户数据成员data的fd成员赋值，指定事件的自定义数据，会随着epoll_wait()返回的事件一并返回

    // 事件准备好后，把它加入到epoll的句柄中
    // 第一个参数：epoll句柄
    // 第二个参数：ctl的动作，这里是添加事件，用EPOLL_CTL_ADD
    // 第三个参数：epoll监听的socket，这里是给监听socket添加epoll，所以填listensock
    // 第四个参数：epoll事件的地址
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listensock, &ev);

    struct epoll_event evs[10];     // 声明一个用于存放epoll返回事件的数组
   
    while (true)
    {
        // 在循环中调用epoll_wait监视sock事件发生
        int infds = epoll_wait(epollfd, evs, 10, -1);

        // 返回失败
        if(infds < 0)
        {
            perror("epoll() failed\n");
            break;
        }

        // 超时，在本程序中，epoll_wait函数最后一个参数-1，不存在超时的情况，但已下代码还是留着‘
        if(infds == 0)
        {
            printf("epoll timeout\n");
            continue;
        }

        // 如果infds > 0.表示有事件发生的socket的数量
        // 遍历epoll返回的已经发生事件的数组evs
        for(int i = 0; i < infds; i++)
        {
            // 注意，先前我们已经将事件的socket通过用户数据成员传递进epoll，那么在这里事件发生的时候，我们就可以通过事件将这个socket带回来
            // 也就是说，我们可以通过这个事件知道是哪个socket发生了事件
            printf("events=%d, data.fd=%d,\n", evs[i].events, evs[i].data.fd);

            // 如果发生的事件是listensock。表示有新的客户端连接上来
            // 监听socket只会用于监听客户端的连接请求，不会接收客户端的数据通信报文
            if(evs[i].data.fd == listensock)
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

                // 为客户端准备可读事件，并添加到epoll中
                ev.data.fd = clientsock;
                ev.events = EPOLLIN;        // 可读事件
                epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsock, &ev);

            }
            else
            {
                // 如果是客户端连接的socket有事件，表示有报文发送过来，或者连接已经断开
                char buffer[1024];      // 存放从客户端读取的数据
                memset(buffer, 0, sizeof(buffer));

                if(recv(evs[i].data.fd, buffer, sizeof(buffer), 0) <= 0)
                {
                    // 客户端连接已经断开
                    printf("client(enventfd = %d) disconnected\n", evs[i].data.fd);
                    close(evs[i].data.fd);
                    // 然后把已经关闭的socket从epoll中删除
                    // 注意，这里已经关闭的socket不需要调用epoll_ctl函数把这个socket从epoll中删除，系统会自动处理已经关闭的socket
                }
                else
                {
                    // 客户端有报文发送过来
                    printf("recv(eventfd=%d):%s\n", evs[i].data.fd, buffer);
                    // 这里把接收到的报文原封不动的发回去
                    send(evs[i].data.fd, buffer, strlen(buffer), 0);
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