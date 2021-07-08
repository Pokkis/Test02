#include "../common/commonsocket.h"
#define MAXBUF 256

int port = 5000;
char *multicast_addr = "224.0.1.1";

int main(int argc, char *argv[])
{
	int s, n;
	struct sockaddr_in srv, cli;
	socklen_t cli_len = sizeof(cli);
	struct ip_mreq mreq;
	char buf[MAXBUF];

	bzero(&srv, sizeof(srv));
	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = htonl(INADDR_ANY);
	srv.sin_port = htons(port);

	//创建一个套接字
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("Opening   socket ");
		return 0;
	}

	//绑定端口
	if (bind(s, (struct sockaddr *)&srv, sizeof(srv)) < 0)
	{
		perror("bind ");
		return 0;
	}

	//设置套接字组播属性
	mreq.imr_multiaddr.s_addr = inet_addr(multicast_addr);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);

	if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
	{
		perror("setsockopt:   IP_ADD_MEMBERSHIP ");
		return 0;
	}

	while (1)
	{
		if ((n = recvfrom(s, buf, MAXBUF, 0, (struct sockaddr *)&cli, &cli_len)) < 0)
		{
			perror("recvfrom ");
			return 0;
		}
		else
		{
			buf[n] = 0;
			fprintf(stdout, "receive   msg   from %s: %s\n ", inet_ntoa(cli.sin_addr), buf);
		}

		strcat(buf, " to you");
		if (sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&cli, (socklen_t)sizeof(cli)) < 0)
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