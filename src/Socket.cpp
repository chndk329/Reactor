#include "Socket.h"

// 创建一个非阻塞的fd
int createnonblocking()
{
    int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(listenfd < 0)
    {
        printf("%s:%s:%d listen socket create error: %d\n", __FILE__, __FUNCTION__, __LINE__, errno); exit(-1);
    }
    return listenfd;
}


Socket::Socket(int fd) : fd_(fd)
{

}

Socket::~Socket()
{
    close(fd_);
}

int Socket::fd() const
{
    return fd_;
}

void Socket::setreuseaddr(bool on)     // 设置SO_REUSEADDR选项
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setreuseport(bool on)     // 设置SO_REUSEPORT选项
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, TCP_NODELAY,  &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::settcpnodelay(bool on)    // 设置TCP_NODELAY选项
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setkeepalive(bool on)     // 设置SO_KEEPALIVE选项
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::bind(const InetAddress& servaddr)
{
    if(::bind(fd_, servaddr.addr(), sizeof(sockaddr)) < 0)
    {
        printf("bind() failed.\n"); exit(-1);
    }

    setipport(servaddr.ip(), servaddr.port());
}

void Socket::listen(int n)
{
    if(::listen(fd_, n) != 0)
    {
        printf("listen() failed.\n"); exit(-1);
    }
}

int Socket::accept(InetAddress& clientaddr)
{
    sockaddr_in peeraddr;
    socklen_t len = sizeof(peeraddr);
                    
    int clientfd = accept4(fd_, (sockaddr *)&peeraddr, &len, SOCK_NONBLOCK);      // accept4可直接设置非阻塞

    clientaddr.setaddr(peeraddr);

    return clientfd;
}

std::string Socket::ip() const
{
    return ip_;
}

uint16_t Socket::port() const
{
    return port_;
}

void Socket::setipport(const std::string &ip, uint16_t port)               // 设置ip和port
{
    ip_ = ip;
    port_ = port;
}