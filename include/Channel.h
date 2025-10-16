#pragma once

#include <functional>
#include <sys/epoll.h>
#include "Epoll.h"
#include "InetAddress.h"
#include "Socket.h"
#include "EventLoop.h"
#include <memory>

class Epoll;
class EventLoop;

class Channel
{
private:
    int fd_ = -1;               // Channel跟fd是一对一的关系

    // Epoll *ep_ = nullptr;       // Channel跟Epoll是多对一的关系，一个Channel只对应一个Epoll
    EventLoop* loop_ = nullptr;

    bool inepoll_ = false;      // Channel是否已添加到epoll树上，如果未添加，调用epoll_ctl()的时候用EPOLL_CTL_ADD，否则用EPOLL_CTL_MOD
    uint32_t events_ = 0;       // fd_需要监视的事件
    uint32_t revents_ = 0;      // fd_已经发生的事件

    std::function<void()> readcallback_;        // fd_读事件的回调函数，将回调Accpetor::newconnection()
    std::function<void()> closecallback_;        // 关闭fd_的回调函数，将回调Connection::closecallback()
    std::function<void()> errorcallback_;        // fd_读事件的回调函数，将回调Connection::errorcallback()
    std::function<void()> writecallback_;        // fd_写事件的回调函数，将回调Connection::writecallback()

public:
    Channel(EventLoop* loop, int fd);
    ~Channel();

    int fd();
    void useet();                   // 采用边缘触发

    void enablereading();           // 让epoll_wait()监视fd的读事件，注册读事件
    void disablereading();          // 注销读事件
    void enablewriting();           // 让epoll_wait()监视fd的写事件，注册写事件
    void disablewriting();          // 注销写事件

    void disabelall();              // 取消全部的事件
    void remove();                  // 从事件循环中删除channel

    void setinepoll();              // 把inepoll_成员的值设置为true
    void setrevents(uint32_t ev);   // 设置revents_成员的值为ev
    bool inepoll();                 // 返回inepoll_
    uint32_t events();              // 返回events_
    uint32_t revents();             // 返回revents_

    void handleevent();             // 事件处理函数，处理epoll_wait()返回的事件

    //void onmessage();                               // 处理对端发送过来的消息

    void setreadcallback(std::function<void()> fn);    // 设置fd_读事件的回调函数
    void setclosecallback(std::function<void()> fn);    // 设置fd_关闭事件的回调函数
    void seterrorcallback(std::function<void()> fn);    // 设置fd_错误事件的回调函数
    void setwritecallback(std::function<void()> fn);    // 设置fd_写事件的回调函数
};