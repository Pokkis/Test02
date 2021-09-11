#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <net/if.h>
#include <errno.h>
#include "../../common/h264tortp.h"



/*
udp客户端
1、创建套接字 socket()
2、sendto()/recvfrom()
3、close()
*/

#define MAXBUF 1024*200
#define SEND_FILE "../../resource/test.h264"
int port = 5454;
//char *udp_addr = "127.0.0.1";
char *udp_addr = "192。168.1.100";

int s;
struct sockaddr_in srv;

#define DEBUG_PRINT         0
#define debug_print(fmt, ...) \
    do { if (DEBUG_PRINT) fprintf(stderr, "-------%s: %d: %s():---" fmt "----\n", \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__);} while (0)

#define DEFAULT_DEST_PORT           1234
#define RTP_PAYLOAD_MAX_SIZE        1400
#define SEND_BUF_SIZE               1500
#define NAL_BUF_SIZE                1500 * 50
#define SSRC_NUM                    10

uint8_t SENDBUFFER[SEND_BUF_SIZE];
uint8_t nal_buf[NAL_BUF_SIZE];

typedef struct _sercer_handle_
{
    int socket;
    struct sockaddr_in svr;
}server_handle;


static int  send_data_to_server(uint8_t *send_buf, size_t len_sendbuf, server_handle handle)
{
    //int n_ret = sendto(s.socket, send_buf, len_sendbuf, 0, (struct sockaddr *)&s.svr, sizeof(s.svr));
    int n_ret = send(s, send_buf, len_sendbuf, 0);
        
    printf("n_ret:%d\n", n_ret);
    return n_ret;
}

static int h264nal2rtp_send(int framerate, uint8_t *pstStream, int nalu_len, server_handle s)
{
    uint8_t *nalu_buf;
    nalu_buf = pstStream;
    // int nalu_len;   /* 不包括0x00000001起始码, 但包括nalu头部的长度 */
    static uint32_t ts_current = 0;
    static uint16_t seq_num = 0;
    rtp_header_t *rtp_hdr;
    nalu_header_t *nalu_hdr;
    fu_indicator_t *fu_ind;
    fu_header_t *fu_hdr;
    size_t len_sendbuf;

    int fu_pack_num;        /* nalu 需要分片发送时分割的个数 */
    int last_fu_pack_size;  /* 最后一个分片的大小 */
    int fu_seq;             /* fu-A 序号 */

    debug_print();
    ts_current += (90000 / framerate);  /* 90000 / 25 = 3600 */

        /*
         * 加入长度判断，
         * 当 nalu_len == 0 时， 必须跳到下一轮循环
         * nalu_len == 0 时， 若不跳出会发生段错误!
         * fix by hmg
         */
        if (nalu_len < 1) {     
            return -1;
        }

        if (nalu_len <= RTP_PAYLOAD_MAX_SIZE) {
            /*
             * single nal unit
             */

            memset(SENDBUFFER, 0, SEND_BUF_SIZE);

            /*
             * 1. 设置 rtp 头
             */
            rtp_hdr = (rtp_header_t *)SENDBUFFER;
            rtp_hdr->csrc_len = 0;
            rtp_hdr->extension = 0;
            rtp_hdr->padding = 0;
            rtp_hdr->version = 2;
            rtp_hdr->payload_type = H264;
		    // rtp_hdr->marker = (pstStream->u32PackCount - 1 == i) ? 1 : 0;   /* 该包为一帧的结尾则置为1, 否则为0. rfc 1889 没有规定该位的用途 */
			rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
            rtp_hdr->timestamp = htonl(ts_current);
            rtp_hdr->ssrc = htonl(SSRC_NUM);

    debug_print();
            /*
             * 2. 设置rtp荷载 single nal unit 头
             */
#if 1
            nalu_hdr = (nalu_header_t *)&SENDBUFFER[12];
            nalu_hdr->f = (nalu_buf[0] & 0x80) >> 7;        /* bit0 */
            nalu_hdr->nri = (nalu_buf[0] & 0x60) >> 5;      /* bit1~2 */
            nalu_hdr->type = (nalu_buf[0] & 0x1f);
    debug_print();
#else
            SENDBUFFER[12] = ((nalu_buf[0] & 0x80))    /* bit0: f */
                | (nalu_buf[0] & 0x60)                 /* bit1~2: nri */
                | (nalu_buf[0] & 0x1f);                /* bit3~7: type */
#endif

            /*
             * 3. 填充nal内容
             */
    debug_print();
            memcpy(SENDBUFFER + 13, nalu_buf + 1, nalu_len - 1);    /* 不拷贝nalu头 */

            /*
             * 4. 发送打包好的rtp到客户端
             */
            len_sendbuf = 12 + nalu_len;
            send_data_to_server(SENDBUFFER, len_sendbuf, s);
    debug_print();
        } else {    /* nalu_len > RTP_PAYLOAD_MAX_SIZE */
            /*
             * FU-A分割
             */
    debug_print();

            /*
             * 1. 计算分割的个数
             *
             * 除最后一个分片外，
             * 每一个分片消耗 RTP_PAYLOAD_MAX_SIZE BYLE
             */
            fu_pack_num = nalu_len % RTP_PAYLOAD_MAX_SIZE ? (nalu_len / RTP_PAYLOAD_MAX_SIZE + 1) : nalu_len / RTP_PAYLOAD_MAX_SIZE;
            last_fu_pack_size = nalu_len % RTP_PAYLOAD_MAX_SIZE ? nalu_len % RTP_PAYLOAD_MAX_SIZE : RTP_PAYLOAD_MAX_SIZE;
            fu_seq = 0;

            for (fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) {
                memset(SENDBUFFER, 0, SEND_BUF_SIZE);

                /*
                 * 根据FU-A的类型设置不同的rtp头和rtp荷载头
                 */
                if (fu_seq == 0) {  /* 第一个FU-A */
                    /*
                     * 1. 设置 rtp 头
                     */
                    rtp_hdr = (rtp_header_t *)SENDBUFFER;
                    rtp_hdr->csrc_len = 0;
                    rtp_hdr->extension = 0;
                    rtp_hdr->padding = 0;
                    rtp_hdr->version = 2;
                    rtp_hdr->payload_type = H264;
		            rtp_hdr->marker = 0;    /* 该包为一帧的结尾则置为1, 否则为0. rfc 1889 没有规定该位的用途 */
			        rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
                    rtp_hdr->timestamp = htonl(ts_current);
                    rtp_hdr->ssrc = htonl(SSRC_NUM);

                    /*
                     * 2. 设置 rtp 荷载头部
                     */
#if 1
                    fu_ind = (fu_indicator_t *)&SENDBUFFER[12];
                    fu_ind->f = (nalu_buf[0] & 0x80) >> 7;
                    fu_ind->nri = (nalu_buf[0] & 0x60) >> 5;
                    fu_ind->type = 28;
#else   /* 下面的错误以后再找 */
                    SENDBUFFER[12] = (nalu_buf[0] & 0x80) >> 7  /* bit0: f */
                        | (nalu_buf[0] & 0x60) >> 4             /* bit1~2: nri */
                        | 28 << 3;                              /* bit3~7: type */
#endif

#if 1
                    fu_hdr = (fu_header_t *)&SENDBUFFER[13];
                    fu_hdr->s = 1;
                    fu_hdr->e = 0;
                    fu_hdr->r = 0;
                    fu_hdr->type = nalu_buf[0] & 0x1f;
#else
                    SENDBUFFER[13] = 1 | (nalu_buf[0] & 0x1f) << 3;
#endif

                    /*
                     * 3. 填充nalu内容
                     */
                    memcpy(SENDBUFFER + 14, nalu_buf + 1, RTP_PAYLOAD_MAX_SIZE - 1);    /* 不拷贝nalu头 */

                    /*
                     * 4. 发送打包好的rtp包到客户端
                     */
                    len_sendbuf = 12 + 2 + (RTP_PAYLOAD_MAX_SIZE - 1);  /* rtp头 + nalu头 + nalu内容 */
                    send_data_to_server(SENDBUFFER, len_sendbuf, s);

                } else if (fu_seq < fu_pack_num - 1) { /* 中间的FU-A */
                    /*
                     * 1. 设置 rtp 头
                     */
                    rtp_hdr = (rtp_header_t *)SENDBUFFER;
                    rtp_hdr->csrc_len = 0;
                    rtp_hdr->extension = 0;
                    rtp_hdr->padding = 0;
                    rtp_hdr->version = 2;
                    rtp_hdr->payload_type = H264;
		            rtp_hdr->marker = 0;    /* 该包为一帧的结尾则置为1, 否则为0. rfc 1889 没有规定该位的用途 */
			        rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
                    rtp_hdr->timestamp = htonl(ts_current);
                    rtp_hdr->ssrc = htonl(SSRC_NUM);

                    /*
                     * 2. 设置 rtp 荷载头部
                     */
#if 1
                    fu_ind = (fu_indicator_t *)&SENDBUFFER[12];
                    fu_ind->f = (nalu_buf[0] & 0x80) >> 7;
                    fu_ind->nri = (nalu_buf[0] & 0x60) >> 5;
                    fu_ind->type = 28;

                    fu_hdr = (fu_header_t *)&SENDBUFFER[13];
                    fu_hdr->s = 0;
                    fu_hdr->e = 0;
                    fu_hdr->r = 0;
                    fu_hdr->type = nalu_buf[0] & 0x1f;
#else   /* 下面的错误以后要找 */
                    SENDBUFFER[12] = (nalu_buf[0] & 0x80) >> 7  /* bit0: f */
                        | (nalu_buf[0] & 0x60) >> 4             /* bit1~2: nri */
                        | 28 << 3;                              /* bit3~7: type */

                    SENDBUFFER[13] = 0 | (nalu_buf[0] & 0x1f) << 3;
#endif

                    /*
                     * 3. 填充nalu内容
                     */
                    memcpy(SENDBUFFER + 14, nalu_buf + RTP_PAYLOAD_MAX_SIZE * fu_seq, RTP_PAYLOAD_MAX_SIZE);    /* 不拷贝nalu头 */

                    /*
                     * 4. 发送打包好的rtp包到客户端
                     */
                    len_sendbuf = 12 + 2 + RTP_PAYLOAD_MAX_SIZE;
                    send_data_to_server(SENDBUFFER, len_sendbuf, s);

                } else { /* 最后一个FU-A */
                    /*
                     * 1. 设置 rtp 头
                     */
                    rtp_hdr = (rtp_header_t *)SENDBUFFER;
                    rtp_hdr->csrc_len = 0;
                    rtp_hdr->extension = 0;
                    rtp_hdr->padding = 0;
                    rtp_hdr->version = 2;
                    rtp_hdr->payload_type = H264;
		            rtp_hdr->marker = 1;    /* 该包为一帧的结尾则置为1, 否则为0. rfc 1889 没有规定该位的用途 */
			        rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
                    rtp_hdr->timestamp = htonl(ts_current);
                    rtp_hdr->ssrc = htonl(SSRC_NUM);

                    /*
                     * 2. 设置 rtp 荷载头部
                     */
#if 1
                    fu_ind = (fu_indicator_t *)&SENDBUFFER[12];
                    fu_ind->f = (nalu_buf[0] & 0x80) >> 7;
                    fu_ind->nri = (nalu_buf[0] & 0x60) >> 5;
                    fu_ind->type = 28;

                    fu_hdr = (fu_header_t *)&SENDBUFFER[13];
                    fu_hdr->s = 0;
                    fu_hdr->e = 1;
                    fu_hdr->r = 0;
                    fu_hdr->type = nalu_buf[0] & 0x1f;
#else   /* 下面的错误以后找 */
                    SENDBUFFER[12] = (nalu_buf[0] & 0x80) >> 7  /* bit0: f */
                        | (nalu_buf[0] & 0x60) >> 4             /* bit1~2: nri */
                        | 28 << 3;                              /* bit3~7: type */

                    SENDBUFFER[13] = 1 << 1 | (nalu_buf[0] & 0x1f) << 3;
#endif

                    /*
                     * 3. 填充rtp荷载
                     */
                    memcpy(SENDBUFFER + 14, nalu_buf + RTP_PAYLOAD_MAX_SIZE * fu_seq, last_fu_pack_size);    /* 不拷贝nalu头 */

                    /*
                     * 4. 发送打包好的rtp包到客户端
                     */
                    len_sendbuf = 12 + 2 + last_fu_pack_size;
                    send_data_to_server(SENDBUFFER, len_sendbuf, s);
                } /* else-if (fu_seq == 0) */
            } /* end of for (fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) */

        } /* end of else-if (nalu_len <= RTP_PAYLOAD_MAX_SIZE) */

    debug_print();
#if 0
        if (nalu_buf) {
            free(nalu_buf);
            nalu_buf = NULL;
        }
#endif

    return 0;
} 

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

char* comm_socket_getIp(int socket)
{
	struct sockaddr_in sockAddr;
	socklen_t addrLen = sizeof(struct sockaddr);
    printf("cdy 11 \n");
	if (0 != getsockname(socket, (struct sockaddr *)&sockAddr, &addrLen))
		return 0;
    printf("cdy 12 \n");
	return inet_ntoa(sockAddr.sin_addr);
}

#define SO_BINDTODEVICE 25
int main(int argc, char *argv[])
{

    char buf[MAXBUF];

    bzero(&srv, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = inet_addr(udp_addr);
    srv.sin_port = htons(port);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        perror("Opening socket ");
        return 0;
    }

    /*绑定网卡*/
    struct ifreq ifr;
    memset(&ifr, 0x00, sizeof(ifr));
    strncpy(ifr.ifr_name, "en6", strlen("en6"));
    setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr));

    /* 设置socket发送和接收缓冲区的大小*/
	int optlen;
	int snd_size = 200*1024;
	optlen = sizeof(snd_size);
	int err = setsockopt(s, SOL_SOCKET, SO_SNDBUF, &snd_size, &optlen);
	if (err < 0)
	{
		printf("set send buff failed！\n");
	}

    if ((connect(s, (const struct sockaddr *)&srv, sizeof(struct sockaddr_in))) == -1) {
        perror("connect");
        exit(-1);
    }

    printf("cdy:%s\n",comm_socket_getIp(s));
    printf("cdy:%s\n",comm_socket_getIp(s));
    printf("cdy:%s\n",comm_socket_getIp(s));
    printf("cdy:%s\n",comm_socket_getIp(s));

    int i = 0;
    int n = 0; //当前读取到的数据大小
    FILE *f_h264 = NULL;
    int n_last = 0; //上一次读取的文件还有多少没有发送的
    int total_len = 0;

    server_handle handle = { 0 };
    handle.socket = s;
    handle.svr = srv;
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
                    if (h264nal2rtp_send(s, (uint8_t *)start_code, end_code - start_code, handle) < 0)
                    {
                        
                        perror("sendto ");
                        return 0;
                    }
                    else
                    {
                        printf("cdy:%s\n",comm_socket_getIp(s));
                        fprintf(stdout, "send type:%d size:%ld send_count:%d\n ", *(start_code+4)&0x1f, end_code - start_code, send_count);
					    send_count++;
					    send_len += end_code - start_code;
					    total_len += end_code - start_code;
					    start_code = end_code;
                    }
					usleep(1000 * 20); 
                }

                if(send_count > 0 && n == MAXBUF)
                {

                    n_last = n - (start_code - buf);
                    memmove(buf, start_code, n_last);
                }
                else
                {
					if (h264nal2rtp_send(25, (uint8_t *)start_code, n - send_len, handle) < 0)
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

        printf("send total_len:%d n:%d\n", total_len, n);
        
        fclose(f_h264);
        usleep(100);
        //break;
    }

    return 1;
}
