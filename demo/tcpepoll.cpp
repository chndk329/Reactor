// 采用epoll模型实现网络通信的服务端
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <netinet/tcp.h>        // TCP_NODELAY需要
#include <signal.h>

#include <sys/timerfd.h>
#include <sys/signalfd.h>

using namespace std;

// 设置非阻塞
void setnonblocking(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        printf("usage:   ./tcpepoll ip port\n");
        printf("example: ./tcpepoll 127.0.0.1 5005\n");
        return -1;
    }

    // 创建用于监听的listenfd
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0)
    {
        printf("socket() failed.\n"); return -1;
    }

    // 设置listenfd属性
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, static_cast<socklen_t>(sizeof opt));
    setsockopt(listenfd, SOL_SOCKET, TCP_NODELAY,  &opt, static_cast<socklen_t>(sizeof opt));
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &opt, static_cast<socklen_t>(sizeof opt));
    setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, &opt, static_cast<socklen_t>(sizeof opt));

    setnonblocking(listenfd);       // 非阻塞

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        printf("bind() failed.\n"); return -1;
    }

    if(listen(listenfd, 128) != 0)
    {
        printf("listen() failed.\n"); return -1;
    }

    int epollfd = epoll_create(1);

    epoll_event ev;
    ev.data.fd = listenfd;
    ev.events = EPOLLIN;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);

    epoll_event evs[1024];

/*
    ////////////////////////////////////////////////////////////////////////////////////////////
    // 定时器加入epoll
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);          // 创建timerfd
    struct itimerspec timeout;
    memset(&timeout, 0, sizeof(struct itimerspec));
    timeout.it_value.tv_sec = 5;            // 定时器为5秒
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(tfd, 0, &timeout, 0);       // 开始计时。alarm(5)
    ev.data.fd = tfd;
    ev.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, tfd, &ev);        // 定时器加入epoll
    ////////////////////////////////////////////////////////////////////////////////////////////
*/

    ////////////////////////////////////////////////////////////////////////////////////////////
    // 信号加入epoll
    sigset_t sigset;                // 创建信号集
    sigemptyset(&sigset);           // 初始化信号集
    sigaddset(&sigset, SIGINT);     // 把信号SIGINT加入信号集
    sigaddset(&sigset, SIGTERM);    // 把信号SIGTERM加入信号集
    sigprocmask(SIG_BLOCK, &sigset, 0);     // 屏蔽当前进程的信号集（当前进程将不会收到信号集中的信号）
    int sigfd = signalfd(-1, &sigset, 0);   // 创建信号集的fd
    ev.data.fd = sigfd;
    ev.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sigfd, &ev);  // 信号集的fd加入epoll
    ////////////////////////////////////////////////////////////////////////////////////////////

    while(1)
    {
        int nfds = epoll_wait(epollfd, evs, 1024, -1);

        if(nfds < 0)
        {
            printf("epoll_wait() failed.\n"); break;
        }

        if(nfds == 0)
        {
            printf("epoll_wait() timeout.\n"); break;
        }

        for(int i = 0; i < nfds; i ++)
        {
            //////////////////////////////////////////////////////////////////////////
            // 处理定时器事件
            // if(evs[i].data.fd == tfd)
            // {
            //     printf("定时器时间已到。\n");
            //     timerfd_settime(tfd, 0, &timeout, 0);       // 重新开始计时，alarm(5)

            //     // 在这里编写处理定时器的代码

            //     continue;
            // }
            //////////////////////////////////////////////////////////////////////////


            //////////////////////////////////////////////////////////////////////////
            // 处理信号事件
            if(evs[i].data.fd == sigfd)
            {
                struct signalfd_siginfo siginfo;
                int s = read(sigfd, &siginfo, sizeof(struct signalfd_siginfo));
                printf("收到信号 = %d。\n", siginfo.ssi_signo);

                // 在这里编写处理信号的代码

                continue;
            }
            //////////////////////////////////////////////////////////////////////////

            if(evs[i].data.fd == listenfd)
            {
                struct sockaddr_in clientaddr;
                socklen_t len = sizeof(clientaddr);
                int clientfd = accept(listenfd, (struct sockaddr *)&clientaddr, &len);

                printf("accpet client(fd = %d, ip = %s, port = %d) ok.\n", clientfd, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

                setnonblocking(clientfd);
                ev.data.fd = clientfd;
                ev.events = EPOLLIN | EPOLLET;      // 边缘触发
                epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev);
            }
            else
            {
                if(evs[i].events & EPOLLRDHUP)                  // 对方已关闭，有些系统监测不到，可以使用EPOLLIN，recv()返回0
                {
                    printf("client(eventfd = %d) disconnected.\n", evs[i].data.fd);
                    close(evs[i].data.fd);  
                    // 调用close的fd会自动从epoll树上删除
                }                     // 普通数据 | 带外数据 
                else if(evs[i].events & (EPOLLIN | EPOLLPRI))   // 接收缓冲区有数据可读
                {
                    char buf[1024];
                    while(1)
                    {
                        memset(buf, 0, sizeof buf);
                        ssize_t nread = read(evs[i].data.fd, buf, sizeof(buf));
                        if(nread > 0)
                        {
                            // 回显
                            printf("recv(eventfd = %d): %s\n", evs[i].data.fd, buf);
                            send(evs[i].data.fd, buf, strlen(buf), 0);
                        }
                        else if(nread == -1 && errno == EINTR)      // 读取数据时被信号中断，继续读取
                        {
                            continue;
                        }
                        else if(nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))       // 全部数据已经读完
                        {
                            break;
                        }
                        else if(nread == 0)     // 对端已经断开
                        {
                            printf("client(eventfd = %d) disconneted.\n", evs[i].data.fd);
                            close(evs[i].data.fd);
                            // 调用close的fd会自动从epoll树上删除
                            break;
                        }
                    }
                }
                else if(evs[i].events & EPOLLOUT)               // 有数据需要写
                {
                    // 写数据
                }
                else                                            // 其他情况视为错误
                {
                    printf("client(eventfd = %d) error.\n", evs[i].data.fd);
                    close(evs[i].data.fd);
                }
            }
        }
    }

    close(listenfd);

    return 0;
}