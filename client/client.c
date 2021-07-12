#include "../common/commonsocket.h"
#define MAXBUF 1024

int port = 5000;
char *multicast_addr = "224.0.1.1";

int main(int argc, char *argv[])
{
    int s;
    struct sockaddr_in srv;
    char buf[MAXBUF];

    //初始化
    bzero(&srv, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = inet_addr(multicast_addr);
    srv.sin_port = htons(port);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        perror("Opening socket ");
        return 0;
    }

    //设置套接字组播属性
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(multicast_addr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        perror("setsockopt:   IP_ADD_MEMBERSHIP ");
        return 0;
    }

    //这里有问题，没有开启组播属性，但是客户端能够收到消息。
    int i = 0;
    int n = 0;
    while (1)
    {
        //fgets(buf, MAXBUF, stdin);
        //sprintf(buf, "hello:%d\n", i);

        if (sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&srv, sizeof(srv)) < 0)
        {
            perror("sendto ");
            return 0;
        }
        else
        {
            fprintf(stdout, "send   to   group   %s   :   %s\n ", multicast_addr, buf);
        }

        sleep(1);
    }

    return 1;
}