#include <stdio.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define MAXBUF 1024*200
#define SEND_FILE "../resource/128x128.264"
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

    //设置套接字组播属性
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(multicast_addr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        perror("setsockopt:   IP_ADD_MEMBERSHIP ");
        return 0;
    }

    /* 设置socket发送和接收缓冲区的大小*/
	int optlen;
	int snd_size = 200*1024;
	optlen = sizeof(snd_size);
	int err = setsockopt(s, SOL_SOCKET, SO_SNDBUF, &snd_size, &optlen);
	if (err < 0)
	{
		printf("set send buff failed！\n");
	}

    int i = 0;
    int n = 0; //当前读取到的数据大小
    FILE *f_h264 = NULL;
    int n_last = 0; //上一次读取的文件还有多少没有发送的
    int total_len = 0;
    while (1)
    {
        //fgets(buf, MAXBUF, stdin);
        //sprintf(buf, "hello:%d\n", i);
        //这里循环读取h264文件发送
        f_h264 = fopen(SEND_FILE, "rb");
		if(NULL == f_h264)
		{
			printf("open file failed errno%d\n",errno);
		}
        int n_start_code = 0;
        while((n = fread(buf + n_last, 1, MAXBUF - n_last, f_h264)) > 0)
        {
            char *start_code = NULL;
            char *end_code = NULL;
            start_code = find_nal_start_code(buf, MAXBUF);
            int send_count = 0;
            n += n_last;
			
            if(start_code)
            {
                int send_len = 0;
                send_count = 0;
                while((end_code = find_nal_start_code(start_code + 4, n - send_len)) > 0)
                {
                    if (sendto(s, start_code, end_code - start_code, 0, (struct sockaddr *)&srv, sizeof(srv)) < 0)
                    {
                        perror("sendto ");
                        return 0;
                    }
                    else
                    {
                        fprintf(stdout, "send type:%d size:%ld send_count:%d\n ", *(start_code+4)&0x1f, end_code - start_code, send_count);
					    send_count++;
					    send_len += end_code - start_code;
					    total_len += end_code - start_code;
					    start_code = end_code;
                    }
					
                }

                if(send_count > 0 && n == MAXBUF)
                {

                    n_last = n - (start_code - buf);
                    memmove(buf, start_code, n_last);
                }
                else
                {
					if (sendto(s, start_code, n - send_len, 0, (struct sockaddr *)&srv, sizeof(srv)) < 0)
                    {
                        perror("sendto ");
                        return 0;
                    }
                    else
                    {
                        total_len += n - send_len;
                        fprintf(stdout, "send type:%d size:%d receive_count:%d\n ", *(start_code+4)&0x1f, n, send_count);
                        n_last = 0;
                    }	
            	}     
			}    
        }

        printf("reveive total_len:%d n:%d\n", total_len, n);
        
        fclose(f_h264);
        sleep(1);
        //break;
    }

    return 1;
}
