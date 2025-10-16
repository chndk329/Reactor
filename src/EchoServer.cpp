#include "EchoServer.h"

EchoServer::EchoServer(const std::string &ip, const uint16_t port, int subthreadnum, int workthreadnum)
 : tcpserver_(ip, port, subthreadnum), threadpool_(new ThreadPool(workthreadnum, "WORKS"))
{
    // 以下代码不是必须的，业务关系什么事件，就指定相应的回调函数
    tcpserver_.setnewconnectioncb(std::bind(&EchoServer::HandleNewConnection, this, std::placeholders::_1));
    tcpserver_.setcloseconnectioncb(std::bind(&EchoServer::HandleClose, this, std::placeholders::_1));
    tcpserver_.seterrorconnectioncb(std::bind(&EchoServer::HandleError, this, std::placeholders::_1));
    tcpserver_.setonmessagecb(std::bind(&EchoServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.setsendcompletecb(std::bind(&EchoServer::HandleSendComplete, this, std::placeholders::_1));
    tcpserver_.settimeoutcb(std::bind(&EchoServer::HandleTimeOut, this, std::placeholders::_1));
}

EchoServer::~EchoServer()
{

}

void EchoServer::Start()
{
    tcpserver_.start();
}

void EchoServer::Stop()        // 停止服务
{
    // 停止工作线程
    threadpool_ -> stop();

    // 终止IO线程（事件循环）
    tcpserver_.stop();
}

void EchoServer::HandleNewConnection(spConnection conn)               // 创建新客户端的连接请求，在TcpServer类中回调此函数
{
    //printf("HandleNewConnection thread(%ld).\n", syscall(SYS_gettid));

    printf("%s, New connection (fd = %d, ip = %s, port = %d) ok\n", Timestamp::now().tostring().c_str(), conn -> fd(), conn -> ip().c_str(), conn -> port());

    // 根据业务需求，在这里增加其他代码
}

void EchoServer::HandleClose(spConnection conn)                         // 关闭客户端的连接，在TcpServer类中回调此函数
{
    //printf("HandleClose thread(%ld).\n", syscall(SYS_gettid));

    printf("%s Connection (eventfd = %d) close.\n", Timestamp::now().tostring().c_str(), conn -> fd());

    // 根据业务需求，在这里增加其他代码
}

void EchoServer::HandleError(spConnection conn)                         // 客户端的连接错误，在TcpServer类中回调此函数
{
    printf("Connection (eventfd = %d) error.\n", conn -> fd());

    // 根据业务需求，在这里增加其他代码
}

void EchoServer::HandleMessage(spConnection conn, std::string &message)  // 处理客户端的请求报文，在TcpServer类中回调此函数
{
    //printf("HandleMessage thread(%ld).\n", syscall(SYS_gettid));
    // 在这里进行业务计算

    // 回显业务
    // message = "回复报文:" + message;
    // 加入发送缓冲区
    // conn -> send(message.data(), message.size());

    if(threadpool_ -> size() == 0)
    {
        // 没有工作线程

        // IO线程直接调用业务处理函数
        OnMessage(conn, message);
    }
    else
    {
        // 把业务添加到线程池的任务队列中
        threadpool_ -> addtask(std::bind(&EchoServer::OnMessage, this, conn, message));
    }
}

void EchoServer::HandleSendComplete(spConnection conn)                  // 数据发送完成后，在TcpServer类中回调此函数
{
    //printf("Message send complete.\n");

    // 根据业务需求，在这里增加其他代码
}

void EchoServer::HandleTimeOut(EventLoop *loop)                        // epoll_wait()超时后，在TcpServer类中回调此函数
{
    //printf("EchoServer timeout.\n");

    // 根据业务需求，在这里增加其他代码
}


void EchoServer::OnMessage(spConnection conn, std::string &message)     // 处理客户端的请求报文（工作线程的任务函数）
{
    // printf("%s Message (eventfd = %d): %s\n", Timestamp::now().tostring().c_str(), conn -> fd(), message.c_str());
    // 回显业务
    message = "回复报文:" + message;
    // std::cout << message << std::endl;
    // 加入发送缓冲区
    conn -> send(message.data(), message.size());
}
