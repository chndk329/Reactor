#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <string>

class InetAddress
{
private:
    sockaddr_in addr_;
public:
    InetAddress();
    InetAddress(const std::string &ip, uint16_t port);              // 如果是监听的fd，用这个构造函数
    InetAddress(const struct sockaddr_in addr);                     // 如果是客户端连上来的fd，用这个构造函数
    ~InetAddress();
    
    const char *ip() const;         // 返回字符串表示的地址，例如：127.0.0.1
    uint16_t    port() const;       // 返回整数表示的端口，例如：8080
    const sockaddr *addr() const;   // 返回addr_成员的地址，转化为sockaddr*
    void setaddr(sockaddr_in clientaddr);   // 设置addr_的值
};