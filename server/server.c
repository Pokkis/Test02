#include "../common/commonsocket.h"
#define MAXBUF 200*1024
#define RECEIVE_FILE  "test.h264"
int port = 5000;
char *multicast_addr = "224.0.1.1";

static char* find_nal_start_code(char *buff, int n_read_len)
{
    if(NULL == buff || n_read_len < 4)
    {
        return NULL;
    }

    int i = 0;
    for(i = 0; i < n_read_len - 4; i++)
    {
        if(buff[i] == 0x00 && buff[i+1] == 0x00 && 
            buff[i+2] == 0x00 && buff[i+3] == 0x01)
        {
            return &buff[i];
        }
    }
    
    return NULL;
}

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

	/* 获取socket发送和接收缓冲区的大小 */
	int optlen;
	int snd_size = 0;
	optlen = sizeof(snd_size);
	int err = getsockopt(s, SOL_SOCKET, SO_SNDBUF, &snd_size, &optlen);
	if (err < 0)
	{
		printf("get send buff failed！\n");
	}
	int rcv_size = 0;
	optlen = sizeof(rcv_size);
	err = getsockopt(s, SOL_SOCKET, SO_RCVBUF, &rcv_size, &optlen);
	if (err < 0)
	{ 
			printf("get rev buff failed!\n");
	}   
		
	printf("senBuffLen：%d\n", snd_size/1024);
	printf("recBuffLen：%d\n", rcv_size/1024);
	FILE *f_h264 = NULL;
	f_h264 = fopen(RECEIVE_FILE, "a+");
	int start = 0;
	while (1)
	{
		int total_len = 0;
        int n = 0; //当前读取到的数据大小
        int n_last = 0; //上一次读取的文件还有多少没有写入的  
        int n_start_code = 0;
		if ((n = recvfrom(s, buf+n_last, MAXBUF - n_last, 0, (struct sockaddr *)&cli, &cli_len)) > 0)
        //while((n = fread(buf + n_last, 1, MAXBUF - n_last, f_h264)) > 0)
        {
            char *start_code = NULL;
            char *end_code = NULL;
            start_code = find_nal_start_code(buf, MAXBUF);
            int receive_count = 0;
            n += n_last;
			if((*(start_code+4)&0x1f) == 7)
			{
				start++;
				printf("start_code:%d start:%d\n", *(start_code+4)&0x1f, start);
				if(start == 15)
				{
					fclose(f_h264);
					break;
				}
			} 
			
            if(start_code)
            {
                int reveive_len = 0;
                receive_count = 0;
                while((end_code = find_nal_start_code(start_code + 4, n - reveive_len)) > 0)
                {
					if((*(end_code+4)&0x1f) == 7)
					{
						start++;
					}

					if(start)
					{
						fwrite(start_code, 1, end_code - start_code, f_h264);
					}
					
					fprintf(stdout, "send type:%d size:%ld send_count:%d\n ", *(start_code+4)&0x1f, end_code - start_code, receive_count);
					receive_count++;
					reveive_len += end_code - start_code;
					total_len += end_code - start_code;
					start_code = end_code;
                }

                if(receive_count > 0 && n == MAXBUF)
                {

                    n_last = n - (start_code - buf);
                    memmove(buf, start_code, n_last);
                }
                else
                {
					if(start < 2)
					{
						
						fwrite(buf + reveive_len, 1, n - reveive_len, f_h264);
					}
					else
					{
						if((*(buf + reveive_len + 4) & 0x1f) != 7 && (*(buf + reveive_len + 4) & 0x1f) != 8)
						{
							fwrite(buf + reveive_len, 1, n - reveive_len, f_h264);
						}
					}

					total_len += n - reveive_len;
					fprintf(stdout, "send type:%d size:%d receive_count:%d\n ", *(start_code+4)&0x1f, n, receive_count);
					n_last = 0;
            	}     
			}    
        }

        printf("reveive total_len:%d n:%d\n", total_len, n);
		
	}

	return 1;
}