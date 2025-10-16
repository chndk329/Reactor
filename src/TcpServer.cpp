#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, const uint16_t port, int threadnum)
 : mainloop_(new EventLoop(true)), threadnum_(threadnum), threadpool_(threadnum_, "IO"), acceptor_(mainloop_.get(), ip, port)
{
    mainloop_ -> setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout, this, std::placeholders::_1));
    acceptor_.setnewconnectioncb(std::bind(&TcpServer::newconnection, this, std::placeholders::_1));

    // 创建从事件循环
    for(int i = 0; i < threadnum_; i ++)
    {
        subloops_.emplace_back(new EventLoop(false, 5, 10));             // 创建从事件循环，放入subloops_容器中
        subloops_[i] -> setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout, this, std::placeholders::_1));      // 设置timeout超时时间
        subloops_[i] -> settimercallback(std::bind(&TcpServer::removeconn, this, std::placeholders::_1));       // 设置删除conn的回调函数
        // 在线程池中启动事件循环（IO线程）
        threadpool_.addtask(std::bind(&EventLoop::run, subloops_[i].get()));
    }
}

TcpServer::~TcpServer()
{
    // stop();
}

void TcpServer::start()
{
    mainloop_ -> run();
}

void TcpServer::stop()        // 停止IO线程和事件循环
{
    // 停止主事件循环
    mainloop_ -> stop();
    printf("主事件循环已停止。\n");
    // 停止从事件循环
    for(auto& loop : subloops_)
    {
        loop -> stop();
    }
    printf("从事件循环已停止。\n");
    // 停止IO线程
    threadpool_.stop();
    printf("IO线程已停止。\n");
}

void TcpServer::newconnection(std::unique_ptr<Socket> clientsock)
{
    // Connection *conn = new Connection(mainloop_, clientsock);

    int clientfd = clientsock -> fd();      // std::move后指针失效
    spConnection conn(new Connection(subloops_[clientfd % threadnum_].get(), std::move(clientsock)));       // 新连接加入从事件循环
    conn -> setclosecallback(std::bind(&TcpServer::closeconnection, this, std::placeholders::_1));
    conn -> seterrorcallback(std::bind(&TcpServer::errorconnection, this, std::placeholders::_1));
    conn -> setonmessagecallback(std::bind(&TcpServer::onmessage, this, std::placeholders::_1, std::placeholders::_2));
    conn -> setsendcompletecallback(std::bind(&TcpServer::sendcomplete, this, std::placeholders::_1));

    { // 加锁
        std::lock_guard<std::mutex> lgd(mutex_);
        conns_[conn -> fd()] = conn;        // 把conn存放在容器中
    }
    subloops_[clientfd % threadnum_] -> newconnection(conn);        // 把conn存放在EventLoop的容器中

    if(newconnectioncb_) newconnectioncb_(conn);        // 回调EchoServer::HandleNewConnection()
}

void TcpServer::closeconnection(spConnection conn)         // 关闭客户端的连接，在Connection类中回调此函数
{
    if(closeconnectioncb_) closeconnectioncb_(conn);

    {   // 加锁
        std::lock_guard<std::mutex> lgd(mutex_);
        conns_.erase(conn -> fd());     // 从map中删除conn
    }
    // delete conn;
}

void TcpServer::errorconnection(spConnection conn)         // 客户端的连接错误，在Connection类中回调此函数
{
    if(errorconnectioncb_) errorconnectioncb_(conn);

    {   // 加锁
        std::lock_guard<std::mutex> lgd(mutex_);
        conns_.erase(conn -> fd());     // 从map中删除conn
    }
    // delete conn;
}

void TcpServer::onmessage(spConnection conn, std::string &message)        // 处理客户端的请求报文，在Connection类中回调此函数
{
    if(onmessagecb_) onmessagecb_(conn, message);
}

void TcpServer::sendcomplete(spConnection conn)            // 数据发送完成后，在Connecion类中回调此函数
{
    if(sendcompletecb_) sendcompletecb_(conn);
}

void TcpServer::epolltimeout(EventLoop *loop)     // epoll_wait()超时后，在EventLoop类中回调此函数
{
    if(timeoutcb_) timeoutcb_(loop);
}

void TcpServer::setnewconnectioncb(std::function<void(spConnection)> fn)
{
    newconnectioncb_ = fn;
}

void TcpServer::setcloseconnectioncb(std::function<void(spConnection)> fn)
{
    closeconnectioncb_ = fn;
}

void TcpServer::seterrorconnectioncb(std::function<void(spConnection)> fn)
{
    errorconnectioncb_ = fn;
}

void TcpServer::setonmessagecb(std::function<void(spConnection, std::string&)> fn)
{
    onmessagecb_ = fn;
}

void TcpServer::setsendcompletecb(std::function<void(spConnection)> fn)
{
    sendcompletecb_ = fn;
}

void TcpServer::settimeoutcb(std::function<void(EventLoop *)> fn)
{
    timeoutcb_ = fn;
}

void TcpServer::removeconn(int fd)            // 删除conns_中的Connection对象，在EventLoop::handletimer()中将回调此函数
{
    {
        std::lock_guard<std::mutex> lgd(mutex_);    // 加锁
        conns_.erase(fd);           // 从map中删除conn
    }
}