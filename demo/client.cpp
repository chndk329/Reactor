// 网络通讯的客户端
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <error.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctime>

using namespace std;

const int N = 100000;

int main(int argc, char *argv[])
{
    if(argc != 3)
    {  
        printf("usage:   ./client ip port\n");
        printf("example: ./client 127.0.0.1 5005\n");
        return -1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buf[1024];

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { printf("socket() error. \n"); return -1; }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        printf("connect(%s:%s) error. \n", argv[2], argv[1]); return -1;
    }

    printf("connect ok.\n");

    // printf("开始时间: %d\n", time(0));

    for(int i = 0; i < N; i ++)
    {
        memset(buf, 0, sizeof buf);
        printf("Input: "); scanf("%s", buf);

        if(send(sockfd, buf, sizeof(buf), 0) <= 0)
        {
            printf("write failed.\n"); close(sockfd); return -1;
        }

        memset(buf, 0, sizeof buf);
        if(recv(sockfd, buf, sizeof(buf), 0) <= 0)
        {
            printf("read failed.\n"); close(sockfd); return -1;
        }

        printf("recv: %s\n", buf);
    }

    // printf("结束时间: %d\n", time(0));

    return 0;
}