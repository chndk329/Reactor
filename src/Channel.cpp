#include "Channel.h"

Channel::Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd)
{

}

Channel::~Channel()
{
    // 析构函数中不能销毁ep_，也不能关闭fd_，因为这两个不属于Channel类，是从外面传进来的，Channel类只是使用他们
}

int Channel::fd()
{
    return fd_;
}

void Channel::useet()                   // 采用边缘触发
{
    events_ |= EPOLLET;
}

void Channel::enablereading()          // 让epoll_wait()监视fd_的读事件
{  
    events_ |= EPOLLIN;
    loop_ -> updatechannel(this);
}

void Channel::disablereading()          // 注销读事件
{
    events_ &= (~EPOLLIN);
    loop_ -> updatechannel(this);
}

void Channel::enablewriting()           // 让epoll_wait()监视fd的写事件，注册写事件
{
    events_ |= EPOLLOUT;
    loop_ -> updatechannel(this);
}

void Channel::disablewriting()          // 注销写事件
{
    events_ &= (~EPOLLOUT);
    loop_ -> updatechannel(this);
}

void Channel::setinepoll()              // 把inepoll_成员的值设置为true
{
    inepoll_ = true;
}

void Channel::setrevents(uint32_t ev)   // 设置revents_成员的值为ev
{
    revents_ = ev;
}

bool Channel::inepoll()                 // 返回inepoll_
{
    return inepoll_;
}

uint32_t Channel::events()              // 返回events_
{
    return events_;
}

uint32_t Channel::revents()             // 返回revents_
{
    return revents_;
}

void Channel::disabelall()              // 取消全部的事件
{
    events_ = 0;
    loop_ -> updatechannel(this);
}

void Channel::remove()                  // 从事件循环中删除channel
{
    disabelall();       // 先取消全部的事件
    loop_ -> removechannel(this);       // 从红黑树上删除fd
}

void Channel::handleevent()             // 事件处理函数，处理epoll_wait()返回的事件
{
    if(revents_ & EPOLLRDHUP)                  // 对方已关闭，有些系统监测不到，可以使用EPOLLIN，recv()返回0
    {
        closecallback_();            //回调Connection::closecallback
    }
    else if(revents_ & (EPOLLIN | EPOLLPRI))   // 接收缓冲区有数据可读
    {
        readcallback_();    //回调Connection::readcallback
    }
    else if(revents_ & EPOLLOUT)               // 有数据需要写
    {
        writecallback_();           //回调Connection::writecallback
    }
    else                                            // 其他情况视为错误
    {
        errorcallback_();       // 回调Connection::errorcallback
    }
}

void Channel::setreadcallback(std::function<void()> fn)    // 设置fd_读事件的回调函数
{
    readcallback_ = fn;
}

void Channel::setclosecallback(std::function<void()> fn)    // 设置fd_关闭事件的回调函数
{
    closecallback_ = fn;
}
void Channel::seterrorcallback(std::function<void()> fn)    // 设置fd_错误事件的回调函数
{
    errorcallback_ = fn;
}

void Channel::setwritecallback(std::function<void()> fn)    // 设置fd_写事件的回调函数
{
    writecallback_ = fn;
}