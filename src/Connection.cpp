#include "Connection.h"

Connection::Connection(EventLoop* loop, std::unique_ptr<Socket> clientsock)
 : loop_(loop), clientsock_(std::move(clientsock)), disconnect_(false), clientchannel_( new Channel(loop_, clientsock_ -> fd())),
   lastatime_(Timestamp())
{
    clientchannel_ -> setreadcallback(std::bind(&Connection::onmessage, this));     // 绑定回调函数
    clientchannel_ -> setclosecallback(std::bind(&Connection::closecallback, this));     // 绑定回调函数
    clientchannel_ -> seterrorcallback(std::bind(&Connection::errorcallback, this));     // 绑定回调函数
    clientchannel_ -> setwritecallback(std::bind(&Connection::writecallback, this));     // 绑定回调函数
    clientchannel_ -> useet();
    clientchannel_ -> enablereading();
}

Connection::~Connection()
{
    // printf("析构啦！！！\n");
}

int Connection::fd() const
{
    return clientsock_ -> fd();
}

std::string Connection::ip() const
{
    return clientsock_ -> ip();
}

uint16_t Connection::port() const
{
    return clientsock_ -> port();
}

void Connection::closecallback()       // TCP连接关闭（断开）的回调函数，供Channel回调
{
    disconnect_ = true;
    clientchannel_ -> remove();         // 删除channel
    closecallback_(shared_from_this());
}

void Connection::errorcallback()       // TCP连接错误的回调函数，供Channel回调
{
    disconnect_ = true;
    clientchannel_ -> remove();         // 删除channel
    errorcallback_(shared_from_this());
}

void Connection::setclosecallback(std::function<void(spConnection)> fn)
{
    closecallback_ = fn;
}
void Connection::seterrorcallback(std::function<void(spConnection)> fn)
{
    errorcallback_ = fn;
}

void Connection::setonmessagecallback(std::function<void(spConnection, std::string&)> fn)
{
    onmessagecallback_ = fn;
}

void Connection::setsendcompletecallback(std::function<void(spConnection)> fn)
{
    sendcompletecallback_ = fn;
}

void Connection::onmessage()                               // 处理对端发送过来的消息
{
    char buf[1024];
    while(1)
    {
        memset(buf, 0, sizeof buf);
        ssize_t nread = read(fd(), buf, sizeof(buf));
        if(nread > 0)
        {
            // 回显
            inputbuffer_.append(buf, nread);
        }
        else if(nread == -1 && errno == EINTR)      // 读取数据时被信号中断，继续读取
        {
            continue;
        }
        else if(nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))       // 全部数据已经读完
        {
            std::string message;
            while(1)
            {
                if(inputbuffer_.pickmessage(message) == false) break;

                // printf("message (eventfd = %d): %s\n", fd(), message.c_str());

                // 已经接到到一个完整的报文，更新时间戳
                lastatime_ = Timestamp::now();

                // 在这里进行计算
                onmessagecallback_(shared_from_this(), message);              // 回调TcpServer::onmessage()
            }
            break;
        }
        else if(nread == 0)     // 对端已经断开
        {
            // clientchannel_ -> remove();             // 删除channel   closecallback()中会调用
            closecallback();
            // 调用close的fd会自动从epoll树上删除
            break;
        }
    }
}

// 处理写事件的回调函数，供Channel回调
void Connection::writecallback()
{
    int writen = ::send(fd(), outputbuffer_.data(), outputbuffer_.size(), 0);       // 尝试把outputbuffer_中的数据全部发送出去
    if(writen > 0) outputbuffer_.erase(0, writen);           // 从outputbuffer_中删除已成果发送的字节数

    if(outputbuffer_.size() == 0) 
    {
        sendcompletecallback_(shared_from_this());
        clientchannel_ -> disablewriting();       // 发送缓冲区已空，不再关注写事件
    }
}

// 有可能在IO线程执行，也有可能在工作线程执行
// 发送数据，不管在任何线程中，都是调用此函数发送数据
void Connection::send(const char *data, size_t size)       //发送数据
{
    if(disconnect_ == true) { printf("客户端连接已断开，send()直接返回。\n"); return; }

    // 因为数据要发送给其它线程处理，所以，把它包装成智能指针。

    // 如果采用裸指针，在调用loop_ -> queueinloop(std::bind(&Connection::sendinloop, this, data, size));后，函数返回。
    // 原始的Connection::onmessage()中的message为局部变量，会被释放，data就会失效，出现野指针
    std::shared_ptr<std::string> message(new std::string(data));

    if(loop_ -> isinloopthread())   // 判断当前线程是否为事件循环的线程（IO线程）
    {
        // 如果当前线程为IO线程，直接执行发送数据的操作
        // printf("send() 在事件循环线程中。\n");
        sendinloop(message);
    }
    else
    {
        // 如果当前线程不是IO线程，调用EventLoop::queueinloop()，把sendinloop()交给事件循环线程（IO线程）去执行
        // printf("send() 不在事件循环线程中。\n");
        // 把发送数据的操作交给IO线程去执行
        loop_ -> queueinloop(std::bind(&Connection::sendinloop, this, message));
    }
}

// 发送数据，如果当前线程为IO线程，直接调用此函数，如果是工作线程，将把此函数传给IO线程去执行
void Connection::sendinloop(std::shared_ptr<std::string> message)
{
    outputbuffer_.appendwithsep(message -> data(), message -> size());
    // 注册写事件
    clientchannel_ -> enablewriting();
}

bool Connection::timeout(time_t now, int timeoutval)           // 判断TCP连接是否超时（空闲太久）
{
    return now - lastatime_.toint() > timeoutval;
}
