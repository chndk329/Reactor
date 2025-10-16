#pragma once

#include <functional>
#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Buffer.h"
#include "Timestamp.h"
#include <atomic>
#include <memory>
#include <sys/syscall.h>


class EventLoop;
class Channel;
class Connection;
using spConnection = std::shared_ptr<Connection>;

class Connection : public std::enable_shared_from_this<Connection>      // 继承这个模板类就可以调用shared_from_this()获取this指针的智能指针
{
private:
    EventLoop* loop_;                           // Connection对应的事件循环，在构造函数中传入
    std::unique_ptr<Socket> clientsock_;        // 服务端用于监听的socket
    std::unique_ptr<Channel> clientchannel_;                    // Connection对应的channel，在构造函数中创建

    Buffer inputbuffer_;                // 接收缓冲区
    Buffer outputbuffer_;               // 发送缓冲区

    std::atomic_bool disconnect_;       // 客户端是否已断开，如果已断开，则设置为true（如果已断开就不发送缓冲区的值）

    std::function<void(spConnection)> closecallback_;       // 关闭fd_的回调函数，将回调TcpServer::closeconnection()
    std::function<void(spConnection)> errorcallback_;       // fd_发送错误的回调函数，将回调TcpServer::errorconnection()
    std::function<void(spConnection, std::string&)> onmessagecallback_;  // 处理报文的回调函数，将回调TcpServer::onmessage()

    std::function<void(spConnection)> sendcompletecallback_;        // 发送数据完成后的回调函数，将回调TcpServer::sendcomplete()

    Timestamp lastatime_;       // 时间戳。创建Connection对象时为当前时间，每接收到一个报文，把时间戳更新为当前时间

public:
    Connection(EventLoop* loop, std::unique_ptr<Socket> clientsock);
    ~Connection();

    int fd() const;
    std::string ip() const;
    uint16_t port() const;

    // 发送数据，不管在任何线程中，都是调用此函数发送数据
    void send(const char *data, size_t size);

    // 发送数据，如果当前线程为IO线程，直接调用此函数，如果是工作线程，将把此函数传给IO线程去执行
    // void sendinloop(const char *data, size_t size);
    void sendinloop(std::shared_ptr<std::string> data);


    void onmessage();           // 处理对端发送过来的消息
    void closecallback();       // TCP连接关闭（断开）的回调函数，供Channel回调
    void errorcallback();       // TCP连接错误的回调函数，供Channel回调
    void writecallback();       // 处理写事件的回调函数，供Channel回调

    void setclosecallback(std::function<void(spConnection)> fn);
    void seterrorcallback(std::function<void(spConnection)> fn);
    void setonmessagecallback(std::function<void(spConnection, std::string&)> fn);

    void setsendcompletecallback(std::function<void(spConnection)> fn);

    bool timeout(time_t now, int timeoutval);           // 判断TCP连接是否超时（空闲太久）
};