#pragma once

#include "Epoll.h"
#include "Connection.h"
#include "Channel.h"
#include <functional>
#include <memory>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <queue>
#include <mutex>
#include <sys/eventfd.h>
#include <sys/timerfd.h>            // 定时器
#include <map>
#include <mutex>
#include <atomic>

class Epoll;
class Channel;
class Connection;

using spConnection = std::shared_ptr<Connection>;

class EventLoop
{
private:
    std::unique_ptr<Epoll> ep_;          // 每个事件循环只有一个Epoll
    std::function<void(EventLoop *)> epolltimeoutcallback_;     // epoll_wait()超时的回调函数

    pid_t threadid_;

    std::queue<std::function<void()>> taskqueue_;       // 事件循环线程被eventfd唤醒后执行的任务队列
    std::mutex mutex_;                                  // 任务队列的互斥锁

    int wakeupfd_;                                      // 用于唤醒事件循环线程的eventfd
    std::unique_ptr<Channel> wakeupchannel_;            // eventfd中的Channel

    int timerfd_;                                       // 定时器的fd
    std::unique_ptr<Channel> timerchannel_;             // 定时器的channel

    bool mainloop_;                                     // true表示主事件循环，false表示从事件循环

    // 清理空闲的Connection
    /*
    1、在事件循环中增加map<int, spConnection> conns_容器，存放运行在该事件循环上的所有Connection对象
    2、如果闹钟时间到了，遍历conns_，判断每个Connection对象是否超时了
    3、如果超时了，从conn_中删除Connection对象
    4、还需要从TcpServer.conns_中删除Connection对象

    5、TcpServer和EventLoop的map容器需要加锁
    6、闹钟时间和超时时间参数化
    */

    std::map<int, spConnection> conns_;         // 存放运行在该事件循环上的所有Connection对象
    std::mutex connsmutex_;                          // 保护conns_的互斥锁

    std::function<void(int)> timercallback_;        // 删除TcpServer中超时的Connection对象，将回调TcpServer::removeconn()

    int timetvl_;    // 闹钟时间间隔，单位：秒
    int timeout_;    // Connection对象的超时时间，单位：秒

    std::atomic_bool stop_;         // 初始值为false，如果设置为true，表示停止事件循环

public:
    EventLoop(bool mainloop, int timetvl = 30, int timeout = 80);            // 创建Epoll对象
    ~EventLoop();                        // 销毁Epoll对象

    void run();             // 运行事件循环
    void stop();            // 停止事件循环

    void updatechannel(Channel *ch);            // 把channel添加或更新到红黑树上，channel中有fd，也有需要监听的事件
    void removechannel(Channel *ch);            // 从红黑树上删除channel
    void setepolltimeoutcallback(std::function<void(EventLoop *)> fn);

    bool isinloopthread();      // 判断当前线程是否为事件循环线程

    void queueinloop(std::function<void()> fn);         // 任务入队函数

    void wakeup();                                      // 用eventfd唤醒事件循环线程

    void handlewakeup();                                // 事件循环线程被唤醒后执行的函数

    void handletimer();                                 // 闹钟响时执行的函数

    void newconnection(spConnection conn);              // 把Connection对象保存到conns_中

    void settimercallback(std::function<void(int)> fn);
};