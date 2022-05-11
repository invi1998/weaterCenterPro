// 程序名：rinetdin.cpp，网络代理服务程序-内网端。
#include "_public.h"

#define MAXSOCK 1024

int clientsockets[MAXSOCK];                // 存放每个socket连接对端的socket的值
// 这个怎么理解呢？
// 假设accept客户端socket是10，然后与目标服务端连接的socket是15，那么在这个数组中，他们的对应关系就是这样存放的
// clientsockets[10] = 15;
// clientsockets[15] = 10;
int clientatime[MAXSOCK];                   // 存放每个socket连接最后一次发送报文的时间
// 这个数组有什么用呢？我们可以用它来判断socket是否连接空闲

CLogFile logfile;
CPActive PActive;

int epollfd;
int timefd;

int cmdconnsock;    // 声明内网程序与外网程序的控制通道

// 向目标端口和IP发起socket连接
int conntodst(const char* dstip, const int dstport);

void EXIT(int sig);   // 进程退出函数。

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("\n");
        printf("Using :./rinetdin logfile ip port\n\n");
        printf("Sample:./rinetdin /tmp/rinetdin.log 192.168.31.133 4000\n\n");
        printf("        /project/tools/bin/procctl 5 /project/tools/bin/rinetdin /tmp/rinetdin.log 192.168.31.133 4000\n\n");
        printf("logfile 本程序运行的日志文件名。\n");
        printf("ip      外网代理服务端的地址。\n");
        printf("port    外网代理服务端的端口。\n\n\n");
        return -1;
    }

    if(logfile.Open(argv[1], "a+") == false)
    {
        printf("打开日志文件失败（%s）。\n",argv[1]);
        return -1;
    }

    /// 关闭全部的信号和输入输出。
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
    // 但请不要用 "kill -9 +进程号" 强行终止。
    CloseIOAndSignal(); signal(SIGINT,EXIT);
    signal(SIGTERM,EXIT);

    // 建立内网程序与外网程序的控制通道
    CTcpClient TcpClien;
    if(TcpClien.ConnectToServer(argv[2], atoi(argv[3])) == false)
    {
        logfile.Write("TcpClien.ConnectToServer(%s, %s) failed\n", argv[2], argv[3]);
        return -1;
    }

    cmdconnsock = TcpClien.m_connfd;

    fcntl(cmdconnsock, F_SETFL, fcntl(cmdconnsock, F_GETFD, 0)|O_NONBLOCK);     // 控制通道设置为非阻塞
    logfile.Write("与外部的控制通道以建立（cmdconnsock = %d)\n", cmdconnsock);

    // 创建epoll句柄
    epollfd = epoll_create(1);

    // 为控制通道socket准备可读事件
    epoll_event ev;             // 声明事件的数据结构

    ev.events = EPOLLIN;
    ev.data.fd = cmdconnsock;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, cmdconnsock, &ev);

    // 创建一个定时器
    timefd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC);       // 创建timerfd

    // 设置定时器的超时时间
    struct itimerspec timeout;
    memset(&timeout, 0, sizeof(struct itimerspec));
    timeout.it_value.tv_sec = 20;       // 超时时间设置为20s
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(timefd, 0, &timeout, NULL);         // 设置定时器

    // 为定时器准备可读事件
    ev.events = EPOLLIN|EPOLLET;            // 读事件，注意，一定要用ET，模式
    ev.data.fd = timefd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, timefd, &ev);

    PActive.AddPInfo(30, "rinetdin");      // 进程心跳的时间，比闹钟时间要长一点

    struct epoll_event evs[10];     // 声明一个用于存放epoll返回事件的数组
   
    while (true)
    {
        // 在循环中调用epoll_wait监视sock事件发生
        int infds = epoll_wait(epollfd, evs, 10, -1);

        // 返回失败
        if(infds < 0)
        {
            logfile.Write("epoll() failed\n");
            break;
        }

        // 超时，在本程序中，epoll_wait函数最后一个参数-1，不存在超时的情况，但已下代码还是留着‘
        if(infds == 0)
        {
            logfile.Write("epoll timeout\n");
            continue;
        }

        // 如果infds > 0.表示有事件发生的socket的数量
        // 遍历epoll返回的已经发生事件的数组evs
        for(int i = 0; i < infds; i++)
        {
            // 注意，先前我们已经将事件的socket通过用户数据成员传递进epoll，那么在这里事件发生的时候，我们就可以通过事件将这个socket带回来
            // 也就是说，我们可以通过这个事件知道是哪个socket发生了事件
            // logfile.Write("events=%d, data.fd=%d,\n", evs[i].events, evs[i].data.fd);

            // ////////////////////////////////////////////////////////////////////////////////////////////////////////
            // 如果定时器的时间已到，设置进程心跳，清理空闲的客户端socket
            if(evs[i].data.fd == timefd)
            {
                timerfd_settime(timefd, 0, &timeout, NULL);         // 从新设置定时器（闹钟只提醒一次，如果不设置，响完这次后，再超时就不会提醒了）

                PActive.UptATime();     // 设置进程心跳

                // 遍历客户端连接的数组，把超时没动作的socket给关闭
                for(int jj = 0; jj < MAXSOCK; jj++)
                {
                    // 如果客户端socket的空闲时间超过80s,就关闭它
                    if((clientsockets[jj] > 0) && (time(0) - clientatime[jj] > 80))
                    {
                        logfile.Write("client(%d, %d) timeout \n", clientsockets[jj], clientsockets[clientsockets[jj]]);
                        close(clientsockets[jj]);
                        close(clientsockets[clientsockets[jj]]);
                        clientsockets[clientsockets[jj]] = 0;   // 然后将记录对端的socket的数组里这两个位置的值置0
                        clientsockets[jj] = 0;                  // 然后将记录对端的socket的数组里这两个位置的值置0
                    }
                }

                continue;
            }

            // ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // 如果发生事件的是控制通道
            if(evs[i].data.fd == cmdconnsock)
            {
                // 读取控制报文的内容
                char buffer[256];
                memset(buffer, 0, sizeof(buffer));
                if(recv(cmdconnsock, buffer, 200, 0) < 0)
                {
                    logfile.Write("与外网的控制通道连接以断开\n");
                    EXIT(-1);
                }

                // 如果接收到的是心跳报文
                if(strcmp(buffer, "<activetest>") == 0)
                {
                    continue;
                }

                // 如果收到的是新建连接的命令

                // 向外网服务端发起连接请求
                int srcsocket = conntodst(argv[2], atoi(argv[3]));
                if(srcsocket < 0)
                {
                    continue;
                }
                if(srcsocket >= MAXSOCK)
                {
                    close(srcsocket);
                    logfile.Write("连接数已经超过最大值%d\n", MAXSOCK);
                    continue;
                }

                // 从控制报文内容中获取目标服务地址和端口
                char dstip[11];
                int dstport;
                GetXMLBuffer(buffer, "dstip", dstip, 30);
                GetXMLBuffer(buffer, "dstport", &dstport);

                // 向目标服务地址和端口发起socket连接
                int dstsocket = conntodst(dstip, dstport);
                if(dstsocket < 0)
                {
                    close(srcsocket);
                    continue;
                }
                if(dstsocket >= MAXSOCK)
                {
                    close(dstsocket);
                    close(srcsocket);
                    logfile.Write("连接数已经超过最大值%d\n", MAXSOCK);
                    continue;
                }

                // 把内网和外网的socket对接在一起
                // 这里是有两个socket了，一个目标sock，一个源socket
                // 为客户端准备可读事件，并添加到epoll中
                ev.data.fd = srcsocket;
                ev.events = EPOLLIN;        // 可读事件
                epoll_ctl(epollfd, EPOLL_CTL_ADD, srcsocket, &ev);

                ev.data.fd = dstsocket;
                ev.events = EPOLLIN;        // 可读事件
                epoll_ctl(epollfd, EPOLL_CTL_ADD, dstsocket, &ev);

                // 这两个socket的对应关系，我们用一个数组来存放
                // 更新clientsockets数组中两端socket的值和活动时间(这里clientsockets存的是对端socket的值)
                clientsockets[srcsocket] = dstsocket;
                clientsockets[dstsocket] = srcsocket;

                clientatime[srcsocket] = time(0);
                clientatime[dstsocket] = time(0);

                logfile.Write("新建内网通道(%d, %d) ok\n", srcsocket, dstsocket);

                continue;
            }


            // //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // 下面的流程是客户端有数据到达的事件，或者连接已经断开
            // 如果是客户端连接的socket有事件，表示有报文发送过来，或者连接已经断开
            // 以下流程是处理内外网通讯链路socket的时间

            char buffer[5000];      // 存放从客户端读取的数据
            int buflen = 0;         // 从socket中读取到的数据的大小

            // 从一端读取数据
            memset(buffer, 0, sizeof(buffer));
            if((buflen = recv(evs[i].data.fd, buffer, sizeof(buffer), 0)) <= 0)
            {
                // 如果连接已经断开，需要关闭两个通道的socket
                logfile.Write("client(%d, %d) disconnected\n", evs[i].data.fd, clientsockets[evs[i].data.fd]);
                close(evs[i].data.fd);                  // 关闭自己的socket
                close(clientsockets[evs[i].data.fd]);   // 关闭对端的socket
                clientsockets[clientsockets[evs[i].data.fd]] = 0;   // 然后将记录对端的socket的数组里这两个位置的值置0
                clientsockets[evs[i].data.fd] = 0;                  // 然后将记录对端的socket的数组里这两个位置的值置0
                // 然后把已经关闭的socket从epoll中删除
                // 注意，这里已经关闭的socket不需要调用epoll_ctl函数把这个socket从epoll中删除，系统会自动处理已经关闭的socket

                continue;
            }
            logfile.Write("from %d to %d , %d bytes \n", evs[i].data.fd, clientsockets[evs[i].data.fd], buflen);
            // 这里把接收到的报文原封不动的发给对端
            send(clientsockets[evs[i].data.fd], buffer, buflen, 0);         // 注意这里填对端的socket
            // 还有这里报文长度填buflen，不要用 strlen(buffer)，因为报文内容可能不是字符串

            // 更新客户端连接的使用时间
            clientatime[evs[i].data.fd] = time(0);
            clientatime[clientsockets[evs[i].data.fd]] = time(0);
        }
    }
    
    return 0;
}

void EXIT(int sig)
{
    logfile.Write("程序退出，sig=%d。\n\n",sig);

    // 关闭全部客户端的socket。
    for (int ii=0;ii<MAXSOCK;ii++)
    {
        if (clientsockets[ii]>0)
        {
            close(clientsockets[ii]);
        }
    }

    close(epollfd);   // 关闭epoll。

    close(timefd);       // 关闭定时器。

    close(cmdconnsock);     // 关闭内网程序与外网程序的控制通道

    exit(0);
}

// 向目标端口和IP发起socket连接
int conntodst(const char* dstip, const int dstport)
{
    // 第一步：创建客户端的socket
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        return -1;
    }

    // 第二步：向服务端发起连接请求
    struct hostent* h;
    if((h = gethostbyname(dstip)) == 0)     // 指定服务端的ip
    {
        logfile.Write("gethostbyname failed\n");
        close(sockfd);
        return -1;
    }

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(dstport);     // 指定服务端的通讯端口
    memcpy(&serveraddr.sin_addr, h->h_addr, h->h_length);

    // 把socket设置为非阻塞
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK);

    // 向服务端发起连接请求
    connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    // if() != 0)
    // {
    //     logfile.Write("connet %s:%d failed\n", dstip, dstport);
    //     close(sockfd);
    //     return -1;
    // }

    return sockfd;

}
