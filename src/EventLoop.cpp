#include "EventLoop.h"

int createtimerfd(int sec = 60)
{
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);          // 创建timerfd
    struct itimerspec timeout;
    memset(&timeout, 0, sizeof(struct itimerspec));
    timeout.it_value.tv_sec = sec;                  // 定时器为sec秒
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(tfd, 0, &timeout, 0);           // 开始计时。alarm(sec)
    return tfd;
}

EventLoop::EventLoop(bool mainloop, int timetvl, int timeout)
 : ep_(new Epoll()), 
   wakeupfd_(eventfd(0, EFD_NONBLOCK)), wakeupchannel_(new Channel(this, wakeupfd_)), 
   timerfd_(createtimerfd(timetvl)), timerchannel_(new Channel(this, timerfd_)), 
   mainloop_(mainloop), 
   timetvl_(timetvl), timeout_(timeout),
   stop_(false)
{
    // 注册eventfd
    wakeupchannel_ -> setreadcallback(std::bind(&EventLoop::handlewakeup, this));
    wakeupchannel_ -> enablereading();

     // 注册timerfd
    timerchannel_ -> setreadcallback(std::bind(&EventLoop::handletimer, this));
    timerchannel_ -> enablereading();
}

EventLoop::~EventLoop()           // 销毁Epoll对象
{

}

void EventLoop::run()             // 运行事件循环
{
    threadid_ = syscall(SYS_gettid);    // 获取事件循环所在线程的id

    while(stop_ == false)
    {
        std::vector<Channel *> channels = ep_ -> loop();        // 等待事件的发生

        // 如果channels为空，表示超时，回调TcpServer::epolltimeout()
        if(channels.empty()) 
        {
            epolltimeoutcallback_(this);
        }
        else
        {
            for(auto& ch : channels)
            {
                ch -> handleevent();
            }
        }
    }
}

void EventLoop::updatechannel(Channel *ch)
{
    ep_ -> updatechannel(ch);
}

void EventLoop::removechannel(Channel *ch)            // 从红黑树上删除channel
{
    ep_ -> removechannel(ch);
}

void EventLoop::setepolltimeoutcallback(std::function<void(EventLoop *)> fn)
{
    epolltimeoutcallback_ = fn;
}

bool EventLoop::isinloopthread()      // 判断当前线程是否为事件循环线程
{
    return syscall(SYS_gettid) == threadid_;
}

void EventLoop::queueinloop(std::function<void()> fn)         // 任务入队函数
{
    {
        std::lock_guard<std::mutex> lgd(mutex_);            // 任务队列加锁
        taskqueue_.push(fn);
    }

    // 任务入队后，唤醒事件循环，使用eventfd
    wakeup();
}

void EventLoop::wakeup()                                      // 用eventfd唤醒事件循环线程
{
    uint64_t val = 1;
    write(wakeupfd_, &val, sizeof(val));
}

void EventLoop::handlewakeup()                                // 事件循环线程被唤醒后执行的函数
{
    // printf("handlewakeup() thread id is %ld\n", syscall(SYS_gettid));

    // 清空eventfd
    // 如果不读取，在水平模型下，eventfd的读事件会一直触发
    uint64_t val;
    read(wakeupfd_, &val, sizeof(val));

    std::function<void()> fn;

    std::lock_guard<std::mutex> lgd(mutex_);            // 任务队列加锁

    // 执行任务队列中的所有发送任务
    while(taskqueue_.size() > 0)
    {
        fn = std::move(taskqueue_.front());
        taskqueue_.pop();
        // 执行任务
        fn();
    }
}

void EventLoop::handletimer()                                 // 闹钟响时执行的函数
{
    // 重新开始计时
    struct itimerspec timeout;
    memset(&timeout, 0, sizeof(struct itimerspec));
    timeout.it_value.tv_sec = timetvl_;                  // 定时器为sec秒
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(timerfd_, 0, &timeout, 0);           // 开始计时。alarm(sec)

    if(mainloop_ == true)
    {
        //printf("主事件循环闹钟时间到了。\n");
    }
    else
    {
        //printf("从事件循环闹钟时间到了。\n");

        // 遍历conns_容器，删除超时的Connection
        time_t now = time(0);       // 获取当前时间
        for(auto it = conns_.begin(); it != conns_.end();)
        {
            if(it -> second -> timeout(now, timeout_))
            {
                timercallback_(it -> first);

                {   // 加锁
                    std::lock_guard<std::mutex> lgd(connsmutex_);
                    it = conns_.erase(it);           // 超时，从conns_中删除Connection
                }
            }
            else
            {
                ++ it;
            }
        }
    }
}

void EventLoop::newconnection(spConnection conn)              // 把Connection对象保存到conns_中
{
    {   // 加锁
        std::lock_guard<std::mutex> lgd(connsmutex_);
        conns_[conn -> fd()] = conn;
    }
}

void EventLoop::settimercallback(std::function<void(int)> fn)
{
    timercallback_ = fn;
}

void EventLoop::stop()            // 停止事件循环
{
    if(stop_) return;

    stop_ = true;

    // 唤醒事件循环
    wakeup();
}