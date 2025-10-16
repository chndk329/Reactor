// 采用epoll模型实现网络通信的服务端
#include "EchoServer.h"
#include <signal.h>

using namespace std;

// 1、设置信号2和15
// 2、在信号处理函数中停止主从时间循环和工作线程
// 3、服务程序主动退出

EchoServer *echoserver;

void Stop(int sig)
{
    printf("sig = %d\n", sig);
    // 调用EchoServer::Stop()函数停止服务
    echoserver -> Stop();
    printf("echoserver已停止。\n");
    delete echoserver;
    exit(0);
}

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        printf("usage:   ./echoserver ip port\n");
        printf("example: ./echoserver 127.0.0.1 5005\n");
        return -1;
    }

    signal(SIGINT, Stop);
    signal(SIGTERM, Stop);

    echoserver = new EchoServer(argv[1], atoi(argv[2]), 2, 3);
    echoserver -> Start();

    return 0;
}
