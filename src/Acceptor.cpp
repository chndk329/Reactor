#include "Acceptor.h"

Acceptor::Acceptor(EventLoop* loop, const std::string &ip, const uint16_t port)
 : loop_(loop), servsock_(createnonblocking()), acceptchannel_(loop_, servsock_.fd())
{
    // servsock_ = new Socket(createnonblocking());
    InetAddress servaddr(ip, port);
    servsock_.setreuseaddr(true);
    servsock_.setkeepalive(true);
    servsock_.setreuseport(true);
    servsock_.settcpnodelay(true);
    
    servsock_.bind(servaddr);
    servsock_.listen();

    //acceptchannel_ = new Channel(loop_, servsock_ -> fd());
    acceptchannel_.setreadcallback(std::bind(&Acceptor::newconnection, this));     // 绑定回调函数
    acceptchannel_.enablereading();
}

Acceptor::~Acceptor()
{
    // delete servsock_;
    // delete acceptchannel_;
}

void Acceptor::newconnection()
{
    InetAddress clientaddr;
    std::unique_ptr<Socket> clientsock(new Socket(servsock_.accept(clientaddr)));
    clientsock -> setipport(clientaddr.ip(), clientaddr.port());

    newconnectioncb_(std::move(clientsock));            // 回调TcpServer::newconnection
}

void Acceptor::setnewconnectioncb(std::function<void(std::unique_ptr<Socket>)> fn)
{
    newconnectioncb_ = fn;
}