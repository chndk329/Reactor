#pragma once

#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>
#include "Channel.h"

class Channel;

class Epoll
{
private:
    static const int MAXEVENTS = 1024;
    int epollfd_ = -1;
    epoll_event events_[MAXEVENTS];
public:
    Epoll();
    ~Epoll();
    
    void updatechannel(Channel *ch);            // 把channel添加或更新到红黑树上，channel中有fd，也有需要监听的事件
    void removechannel(Channel *ch);            // 从红黑树上删除channel
    std::vector<Channel *> loop(int timeout = -1);    // 运行epoll_wait()，等待事件的发生，已发生的事件用vector容器返回
    
};
