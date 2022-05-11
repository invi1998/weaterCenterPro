// 程序名：rinetd.cpp，网络代理服务程序-外网端。
#include "_public.h"

#define MAXSOCK 1024

// # 参数格式  监听端口    目标IP  目标端口
// 3005    192.168.31.133  5505    # demo02程序的通讯端口
// 3006    192.168.31.133  3306    # mysql数据库的端口
// 3007    192.168.31.133  1521    # Oracle数据库的端口
// 3008    192.168.31.133  22      # ssh协议的端口
// 3009    192.168.31.133  8080    # 数据服务总线的端口

// 声明代理路由参数的结构体
struct st_route
{
    int listenport;         // 本地监听的通讯端口
    char dstip[31];         // 目标主机的ip地址
    int dstport;             // 目标主机的通讯端口
    int listensock;         // 本地监听的socket
}stroute;

vector<struct st_route> vroute;         // 代理路由的容器

int clientsockets[MAXSOCK];                // 存放每个socket连接对端的socket的值
// 这个怎么理解呢？
// 假设accept客户端socket是10，然后与目标服务端连接的socket是15，那么在这个数组中，他们的对应关系就是这样存放的
// clientsockets[10] = 15;
// clientsockets[15] = 10;
int clientatime[MAXSOCK];                   // 存放每个socket连接最后一次发送报文的时间
// 这个数组有什么用呢？我们可以用它来判断socket是否连接空闲

CLogFile logfile;
CPActive PActive;

int epollfd;        // epoll句柄
int timefd;         // 定时器句柄

int cmdlistensock = 0;          // 外网服务端监听内网客户端的socket
int cmdconnsock = 0;            // 内网客户端与服务端的控制通道

bool loadrout(const char* inifile);     // 把代理路由参数加载到vroute容器

// 初始化服务端的监听端口
int initserver(int port);

void EXIT(int sig);   // 进程退出函数。

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("\n");
        printf("Using :./rinetd logfile inifile cmdport\n\n");
        printf("Sample:./rinetd /tmp/rinetd.log /etc/rinetd.conf 4000\n\n");
        printf("        /project/tools/bin/procctl 5 /project/tools/bin/rinetd /tmp/rinetd.log /etc/rinetd.conf 4000\n\n");
        printf("logfile 本程序运行的日志文件名。\n");
        printf("inifile 代理服务参数配置文件。\n");
        printf("cmdport 与内网代理程序的通讯端口。\n\n");
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

    // 把代理路由参数加载到vroute容器中
    if(loadrout(argv[2]) == false)
    { 
        return -1;
    }

    logfile.Write("加载代理路由参数成功（%d)\n", vroute.size());

    // 初始化监听内容程序的端口，等待内网发起连接，建立控制通道
    if((cmdlistensock = initserver(atoi(argv[3]))) < 0)
    {
        logfile.Write("初始化内网监听端口(%s)失败\n", argv[3]);
        EXIT(-1);
    }

    // 等待内网程序的连接请求，建立连接控制通道
    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    cmdconnsock = accept(cmdlistensock, (struct sockaddr*)&client, &len);
    if(cmdconnsock < 0)
    {
        logfile.Write("accept() failed\n");
        EXIT(-1);
    }
    logfile.Write("与内部控制通道已建立（cmdconsock = %d)\n", cmdconnsock);

    // 遍历容器，初始化监听的socket
    for(auto iter = vroute.begin(); iter != vroute.end(); ++iter)
    {
        if(((*iter).listensock = initserver((*iter).listenport)) < 0)
        {
            logfile.Write("初始化监听端口(%d)失败\n", (*iter).listenport);
            EXIT(-1);
        }

        // 把监听的socket设置为非阻塞
        fcntl((*iter).listensock, F_SETFL, fcntl((*iter).listensock, F_GETFD, 0)|O_NONBLOCK);

    }

    // 创建epoll句柄
    epollfd = epoll_create(1); 

    // 为监听socket准备可读事件
    epoll_event ev;             // 声明事件的数据结构
    for(auto iter = vroute.begin(); iter != vroute.end(); ++iter)
    {
        ev.events = EPOLLIN;        // 对结构体的events成员赋值，表示关心读事件
        ev.data.fd = (*iter).listensock;    // 对结构体的用户数据成员data的fd成员赋值，指定事件的自定义数据，会随着epoll_wait()返回的事件一并返回
        
        // 事件准备好后，把它加入到epoll的句柄中
        // 第一个参数：epoll句柄
        // 第二个参数：ctl的动作，这里是添加事件，用EPOLL_CTL_ADD
        // 第三个参数：epoll监听的socket，这里是给监听socket添加epoll，所以填listensock
        // 第四个参数：epoll事件的地址
        epoll_ctl(epollfd, EPOLL_CTL_ADD, (*iter).listensock, &ev);
    }

    // 注意，监听内网程序的cmdlistensock和控制通道的cmdconnsock是阻塞的，也不需要epoll进行管理

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

    PActive.AddPInfo(30, "rinetd");      // 进程心跳的时间，比闹钟时间要长一点

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
            // 除此之外，还需要做一件事情，就是根内网的程序做心跳，这个心跳不是进程的心跳，是网络报文的心跳
            if(evs[i].data.fd == timefd)
            {
                timerfd_settime(timefd, 0, &timeout, NULL);         // 从新设置定时器（闹钟只提醒一次，如果不设置，响完这次后，再超时就不会提醒了）

                PActive.UptATime();     // 设置进程心跳

                // 通过控制通道向内网程序发送心跳报文
                char buffer[256];
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "<activetest>");
                if(send(cmdconnsock, buffer, strlen(buffer), 0) <= 0)
                {
                    logfile.Write("与内网程序的控制通道已经断开\n");
                    EXIT(-1);
                }

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

            // 如果发生事件的是监听外网的listensock，表示外网有新的客户端连上来
            auto iter = vroute.begin();
            for(; iter != vroute.end(); ++iter)
            {
                if(evs[i].data.fd == (*iter).listensock)
                {
                    // 接收外网客户端的连接
                    struct sockaddr_in client;
                    socklen_t len = sizeof(client);
                    int srcsocket = accept((*iter).listensock, (struct sockaddr*)&client, &len);        // 源端socket
                    if(srcsocket < 0)
                    {
                        logfile.Write("accept() failed\n");
                        break;;
                    }
                    if(srcsocket >= MAXSOCK)
                    {
                        logfile.Write("连接数已经超过最大值%d\n", MAXSOCK);
                        close(srcsocket);
                        break;
                    }

                    // 通过控制通道向内网程序发送命令，把路由参数传递给它
                    char buffer[256];
                    memset(buffer, 0, sizeof(buffer));
                    sprintf(buffer, "<dstip>%s</dstip><dstport>%d</dstport>", (*iter).dstip, (*iter).dstport);
                    // 把目标ip和目标端口通过控制通道发送给内网代理程序
                    if(send(cmdconnsock, buffer, strlen(buffer), 0) < 0)
                    {
                        logfile.Write("与内网的控制通道已经断开连接\n");
                        EXIT(-1);
                    } 

                    // 接受内网代理程序的连接
                    int dstsocket = accept(cmdlistensock, (struct sockaddr*)&client, &len);
                    if(dstsocket < 0)
                    {
                        close(srcsocket);
                        break;
                    }

                    if(dstsocket >= MAXSOCK)
                    {
                        logfile.Write("连接数已经超过最大值%d\n", MAXSOCK);
                        close(dstsocket);
                        close(srcsocket);
                        break;
                    }

                    // 把内网和外网客户端的socket对接在一起

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

                    logfile.Write("accept port %d client(%d, %d) ok\n", (*iter).listenport, srcsocket, dstsocket);

                    break;
                }
                
            }

            // 如果 iter < vroute.end() 表示事件已经在上面的循环中被处理了
            if(distance(iter, vroute.begin()) < vroute.size())
            {
                continue;
            }

            // //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // 下面的流程是客户端有数据到达的事件，或者连接已经断开
            // 如果是客户端连接的socket有事件，表示有报文发送过来，或者连接已经断开

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

// 把代理路由参数加载到vroute容器
bool loadrout(const char* inifile)
{
    CFile File;

    if(File.Open(inifile, "r") == false)
    {
        logfile.Write("打开代理路由参数文件（%s）失败\n", inifile);
        return false;
    }

    char strBuffer[256];
    CCmdStr Cmdstr;

    while (true)
    {
        memset(strBuffer, 0, sizeof(strBuffer));

        if(File.FFGETS(strBuffer, 200) == false)
        {
            break;
        }

        char *pos = strstr(strBuffer, "#");

        if(pos!=0)
        {
            pos[0] = 0;         // 删除文字说明
        }

        DeleteRChar(strBuffer, ' ');        // 删除右边的空格

        UpdateStr(strBuffer, "  ", " ", true);       // 把两个空格替换成一个空格，注意第三个参数

        Cmdstr.SplitToCmd(strBuffer, " ");
        if(Cmdstr.CmdCount() != 3) continue;

        memset(&stroute, 0, sizeof(struct st_route));

        Cmdstr.GetValue(0, &stroute.listenport);
        Cmdstr.GetValue(1, stroute.dstip);
        Cmdstr.GetValue(2, &stroute.dstport);

        vroute.push_back(stroute);
    }

    return true;
    
}

void EXIT(int sig)
{
    logfile.Write("程序退出，sig=%d。\n\n",sig);

    // 关闭全部监听的socket。
    for (int ii=0;ii<vroute.size();ii++)
    {
        close(vroute[ii].listensock);
    }

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

    close(cmdlistensock);       // 关闭监听内网程序的socket

    close(cmdconnsock);         // 关闭内网程序与外网程序的控制通道

    exit(0);
}

