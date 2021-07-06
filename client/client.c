#include "../common/commonsocket.h"
#define MAXBUF 256

int port = 5000;
char *multicast_addr = "224.0.1.1 ";

int main(int argc, char *argv[])
{
    int s;
    struct sockaddr_in srv;
    char buf[MAXBUF];

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

    while (1)
    {
        fgets(buf, MAXBUF, stdin);
        if (sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&srv, sizeof(srv)) < 0)
        {
            perror("sendto ");
            return 0;
        }
        else
        {
            fprintf(stdout, "send   to   group   %s   :   %s\n ", multicast_addr, buf);
        }
    }

    return 1;
}