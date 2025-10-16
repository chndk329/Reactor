#include "Epoll.h"

Epoll::Epoll()
{
    if((epollfd_ = epoll_create(1)) == -1)
    {
        printf("epoll_create() failed(%d)\n", errno); exit(-1);
    }
}

Epoll::~Epoll()
{
    close(epollfd_);
}

void Epoll::updatechannel(Channel *ch)
{
    epoll_event ev;
    ev.data.ptr = ch;
    ev.events = ch -> events();
    
    if(ch -> inepoll())
    {
        if(epoll_ctl(epollfd_, EPOLL_CTL_MOD, ch -> fd(), &ev) == -1)
        {
            printf("epoll_ctl() error.\n"); exit(-1);
        }
    }
    else
    {
        if(epoll_ctl(epollfd_, EPOLL_CTL_ADD, ch -> fd(), &ev) == -1)
        {
            printf("epoll_ctl() error.\n"); exit(-1);
        }
        ch -> setinepoll();     //标记已经加入epoll树上
    }
}

void Epoll::removechannel(Channel *ch)            // 从红黑树上删除channel
{
    if(ch -> inepoll())
    {
        // 删除channel
        if(epoll_ctl(epollfd_, EPOLL_CTL_DEL, ch -> fd(), 0) == -1)
        {
            printf("epoll_ctl() error.\n"); exit(-1);
        }
    }
}

std::vector<Channel *> Epoll::loop(int timeout)    // 运行epoll_wait()，等待事件的发生，已发生的事件用vector容器返
{
    std::vector<Channel *> channels;

    memset(events_, 0, sizeof events_);
    int nfds = epoll_wait(epollfd_, events_, MAXEVENTS, timeout);

    // 返回失败
    if(nfds < 0)
    {
        // EBADF: epfd不是一个有效的描述符
        // EFAULT: 参数events指向内存区域不可写
        // EINVAL: epfd不是一个epoll文件描述符，或者参数maxevents小于等于0
        // EINTR: 阻塞过程中被信号中断，epoll_pwait()可以避免，或者错误处理中，解析error后重新调用epoll_wait()
        // 在Reactor模型中，不建议使用信号，因为信号处理起来很麻烦，没必要
        printf("epoll_wait() failed.\n"); exit(-1);
    }

    if(nfds == 0)
    {
        // 如果epoll_wait()超时，表示系统很空闲，返回的channels为空
        return channels;
    }

    // 有事件发生
    for(int i = 0; i < nfds; i ++)
    {
        Channel *ch = (Channel *)events_[i].data.ptr;
        ch -> setrevents(events_[i].events);            // 设置channel的revents_成员
        channels.push_back(ch);
    }

    return channels;
}