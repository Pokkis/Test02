#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include "h264tortp.h"
#include "debug_log.h"

#define DEBUG_PRINT         1
#define debug_print(fmt, ...) \
    do { if (DEBUG_PRINT) fprintf(stderr, "-------%s: %d: %s():---" fmt "----\n", \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__);} while (0)


#define RTP_PAYLOAD_MAX_SIZE    1400
#define MAX_NAL_BUF_SIZE        (1500 * 100)
#define SO_MAXCONN              10
#define DEFAULT_PORT            1893
#define SEND_BUF_SIZE           1500
#define SSRC_NUM                10

uint8_t SENDBUFFER[SEND_BUF_SIZE];
static int USE_TCP = 1; /* 1: TCP; 0: UDP */
static int sock_opt = 1;
static int TCP_NODELAY_FLAG = 0;

int bind_server(int server_s, const char *server_ip, uint16_t server_port);
int bind_server(int server_s, const char *server_ip, uint16_t server_port)
{
    struct sockaddr_in server_sockaddr;

    memset(&server_sockaddr, 0, sizeof server_sockaddr);
    server_sockaddr.sin_family = AF_INET;
    if (server_ip)
        inet_aton(server_ip, &server_sockaddr.sin_addr);
    else
        server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    server_sockaddr.sin_port = htons(server_port);

    return bind(server_s, (struct sockaddr *)&server_sockaddr,
                sizeof(server_sockaddr));
}

static int create_server_socket(const char *server_ip, uint16_t server_port);
static int create_server_socket(const char *server_ip, uint16_t server_port)
{
    int server_s;
    int ret;
    struct sockaddr_in server_sockaddr;

    memset(&server_sockaddr, 0, sizeof server_sockaddr);
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(server_port);
    if (server_ip)
        inet_aton(server_ip, &server_sockaddr.sin_addr);

    if (USE_TCP)
        server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    else     /* use udp */
        server_s = socket(AF_INET, SOCK_DGRAM, 0);

    if (server_s == -1) {
        debug_log("unable to create socket");
        exit(1);
    }

    if (TCP_NODELAY_FLAG) {
        ret = setsockopt(server_s, IPPROTO_TCP, TCP_NODELAY, 
                         (void *)&sock_opt, sizeof(sock_opt));
        if (ret == -1) {
            debug_log("setsockopt");
            exit(1);
        }
    }
    ret = connect(server_s, (const struct sockaddr *)&server_sockaddr,
                  sizeof(server_sockaddr));
    if (ret) {
        debug_log("connect");
        exit(1);
    }

    debug_print();
    return server_s;
}

static int copy_nal_from_file(FILE *fp, uint8_t *buf, int *len);
static int copy_nal_from_file(FILE *fp, uint8_t *buf, int *len)
{
    char tmpbuf[4];     /* i have forgotten what this var mean */
    char tmpbuf2[1];    /* i have forgotten what this var mean */
    int flag = 0;       /* i have forgotten what this var mean */
    int ret;

    *len = 0;

    do {
        ret = fread(tmpbuf2, 1, 1, fp);
        if (!ret) {
            return -1;
        }
        if (!flag && tmpbuf2[0] != 0x0) {
            buf[*len] = tmpbuf2[0];
            (*len)++;
            // debug_print("len is %d", *len);
        } else if (!flag && tmpbuf2[0] == 0x0) {
            flag = 1;
            tmpbuf[0] = tmpbuf2[0];
            debug_log("len is %d", *len);
        } else if (flag) {
            switch (flag) {
            case 1:
                if (tmpbuf2[0] == 0x0) {
                    flag++;
                    tmpbuf[1] = tmpbuf2[0];
                } else {
                    flag = 0;
                    buf[*len] = tmpbuf[0];
                    (*len)++;
                    buf[*len] = tmpbuf2[0];
                    (*len)++;
                }
                break;
            case 2:
                if (tmpbuf2[0] == 0x0) {
                    flag++;
                    tmpbuf[2] = tmpbuf2[0];
                } else if (tmpbuf2[0] == 0x1) {
                    flag = 0;
                    return *len;
                } else {
                    flag = 0;
                    buf[*len] = tmpbuf[0];
                    (*len)++;
                    buf[*len] = tmpbuf[1];
                    (*len)++;
                    buf[*len] = tmpbuf2[0];
                    (*len)++;
                }
                break;
            case 3:
                if (tmpbuf2[0] == 0x1) {
                    flag = 0;
                    return *len;
                } else {
                    flag = 0;
                    break;
                }
            }
        }

    } while (1);

    return *len;
} /* static int copy_nal_from_file(FILE *fp, char *buf, int *len) */

static void send_data_to_client_list(uint8_t *send_buf, size_t len_sendbuf, int sock_fd)
{
    int ret;

    ret = send(sock_fd, send_buf, len_sendbuf, 0);
    if (ret < 0) {
        debug_print("send fail");
    }
    usleep(2);

    return;
} /* void send_data_to_client_list(uint8_t *send_buf, size_t len_sendbuf, linklist client_ip_list) */

static int h264naltortp_send_tcp(int framerate, uint8_t *pstStream, int nalu_len, int sock_fd)
{
    uint8_t *nalu_buf;
    nalu_buf = pstStream;
    // int nalu_len;   /* ?????????0x00000001?????????, ?????????nalu??????????????? */
    static uint32_t ts_current = 0;
    static uint16_t seq_num = 0;
    rtp_header_t *rtp_hdr;
    nalu_header_t *nalu_hdr;
    fu_indicator_t *fu_ind;
    fu_header_t *fu_hdr;
    size_t len_sendbuf;

    int fu_pack_num;        /* nalu ???????????????????????????????????? */
    int last_fu_pack_size;  /* ??????????????????????????? */
    int fu_seq;             /* fu-A ?????? */

    ts_current += (90000 / framerate);  /* 90000 / 25 = 3600 */

        /*
         * ?????????????????????
         * ??? nalu_len == 0 ?????? ???????????????????????????
         * nalu_len == 0 ?????? ??????????????????????????????!
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
             * 1. ?????? rtp ???
             */
            rtp_hdr = (rtp_header_t *)SENDBUFFER;
            rtp_hdr->csrc_len = 0;
            rtp_hdr->extension = 0;
            rtp_hdr->padding = 0;
            rtp_hdr->version = 2;
            rtp_hdr->payload_type = H264;
		    // rtp_hdr->marker = (pstStream->u32PackCount - 1 == i) ? 1 : 0;   /* ?????????????????????????????????1, ?????????0. rfc 1889 ??????????????????????????? */
			rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
            rtp_hdr->timestamp = htonl(ts_current);
            rtp_hdr->ssrc = htonl(SSRC_NUM);

            /*
             * 2. ??????rtp?????? single nal unit ???
             */
#if 1
            nalu_hdr = (nalu_header_t *)&SENDBUFFER[12];
            nalu_hdr->f = (nalu_buf[0] & 0x80) >> 7;        /* bit0 */
            nalu_hdr->nri = (nalu_buf[0] & 0x60) >> 5;      /* bit1~2 */
            nalu_hdr->type = (nalu_buf[0] & 0x1f);
#else
            SENDBUFFER[12] = ((nalu_buf[0] & 0x80))    /* bit0: f */
                | (nalu_buf[0] & 0x60)                 /* bit1~2: nri */
                | (nalu_buf[0] & 0x1f);                /* bit3~7: type */
#endif

            /*
             * 3. ??????nal??????
             */
            memcpy(SENDBUFFER + 13, nalu_buf + 1, nalu_len - 1);    /* ?????????nalu??? */

            /*
             * 4. ??????????????????rtp????????????
             */
            len_sendbuf = 12 + nalu_len;
            send_data_to_client_list(SENDBUFFER, len_sendbuf, sock_fd);
        } else {    /* nalu_len > RTP_PAYLOAD_MAX_SIZE */
            /*
             * FU-A??????
             */

            /*
             * 1. ?????????????????????
             *
             * ???????????????????????????
             * ????????????????????? RTP_PAYLOAD_MAX_SIZE BYLE
             */
            fu_pack_num = nalu_len % RTP_PAYLOAD_MAX_SIZE ? (nalu_len / RTP_PAYLOAD_MAX_SIZE + 1) : nalu_len / RTP_PAYLOAD_MAX_SIZE;
            last_fu_pack_size = nalu_len % RTP_PAYLOAD_MAX_SIZE ? nalu_len % RTP_PAYLOAD_MAX_SIZE : RTP_PAYLOAD_MAX_SIZE;
            fu_seq = 0;

            for (fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) {
                memset(SENDBUFFER, 0, SEND_BUF_SIZE);

                /*
                 * ??????FU-A????????????????????????rtp??????rtp?????????
                 */
                if (fu_seq == 0) {  /* ?????????FU-A */
                    /*
                     * 1. ?????? rtp ???
                     */
                    rtp_hdr = (rtp_header_t *)SENDBUFFER;
                    rtp_hdr->csrc_len = 0;
                    rtp_hdr->extension = 0;
                    rtp_hdr->padding = 0;
                    rtp_hdr->version = 2;
                    rtp_hdr->payload_type = H264;
		            rtp_hdr->marker = 0;    /* ?????????????????????????????????1, ?????????0. rfc 1889 ??????????????????????????? */
			        rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
                    rtp_hdr->timestamp = htonl(ts_current);
                    rtp_hdr->ssrc = htonl(SSRC_NUM);

                    /*
                     * 2. ?????? rtp ????????????
                     */
#if 1
                    fu_ind = (fu_indicator_t *)&SENDBUFFER[12];
                    fu_ind->f = (nalu_buf[0] & 0x80) >> 7;
                    fu_ind->nri = (nalu_buf[0] & 0x60) >> 5;
                    fu_ind->type = 28;
#else   /* ??????????????????????????? */
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
                     * 3. ??????nalu??????
                     */
                    memcpy(SENDBUFFER + 14, nalu_buf + 1, RTP_PAYLOAD_MAX_SIZE - 1);    /* ?????????nalu??? */

                    /*
                     * 4. ??????????????????rtp???????????????
                     */
                    len_sendbuf = 12 + 2 + (RTP_PAYLOAD_MAX_SIZE - 1);  /* rtp??? + nalu??? + nalu?????? */
                    send_data_to_client_list(SENDBUFFER, len_sendbuf, sock_fd);

                } else if (fu_seq < fu_pack_num - 1) { /* ?????????FU-A */
                    /*
                     * 1. ?????? rtp ???
                     */
                    rtp_hdr = (rtp_header_t *)SENDBUFFER;
                    rtp_hdr->csrc_len = 0;
                    rtp_hdr->extension = 0;
                    rtp_hdr->padding = 0;
                    rtp_hdr->version = 2;
                    rtp_hdr->payload_type = H264;
		            rtp_hdr->marker = 0;    /* ?????????????????????????????????1, ?????????0. rfc 1889 ??????????????????????????? */
			        rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
                    rtp_hdr->timestamp = htonl(ts_current);
                    rtp_hdr->ssrc = htonl(SSRC_NUM);

                    /*
                     * 2. ?????? rtp ????????????
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
#else   /* ??????????????????????????? */
                    SENDBUFFER[12] = (nalu_buf[0] & 0x80) >> 7  /* bit0: f */
                        | (nalu_buf[0] & 0x60) >> 4             /* bit1~2: nri */
                        | 28 << 3;                              /* bit3~7: type */

                    SENDBUFFER[13] = 0 | (nalu_buf[0] & 0x1f) << 3;
#endif

                    /*
                     * 3. ??????nalu??????
                     */
                    memcpy(SENDBUFFER + 14, nalu_buf + RTP_PAYLOAD_MAX_SIZE * fu_seq, RTP_PAYLOAD_MAX_SIZE);    /* ?????????nalu??? */

                    /*
                     * 4. ??????????????????rtp???????????????
                     */
                    len_sendbuf = 12 + 2 + RTP_PAYLOAD_MAX_SIZE;
                    send_data_to_client_list(SENDBUFFER, len_sendbuf, sock_fd);

                } else { /* ????????????FU-A */
                    /*
                     * 1. ?????? rtp ???
                     */
                    rtp_hdr = (rtp_header_t *)SENDBUFFER;
                    rtp_hdr->csrc_len = 0;
                    rtp_hdr->extension = 0;
                    rtp_hdr->padding = 0;
                    rtp_hdr->version = 2;
                    rtp_hdr->payload_type = H264;
		            rtp_hdr->marker = 1;    /* ?????????????????????????????????1, ?????????0. rfc 1889 ??????????????????????????? */
			        rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
                    rtp_hdr->timestamp = htonl(ts_current);
                    rtp_hdr->ssrc = htonl(SSRC_NUM);

                    /*
                     * 2. ?????? rtp ????????????
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
#else   /* ???????????????????????? */
                    SENDBUFFER[12] = (nalu_buf[0] & 0x80) >> 7  /* bit0: f */
                        | (nalu_buf[0] & 0x60) >> 4             /* bit1~2: nri */
                        | 28 << 3;                              /* bit3~7: type */

                    SENDBUFFER[13] = 1 << 1 | (nalu_buf[0] & 0x1f) << 3;
#endif

                    /*
                     * 3. ??????rtp??????
                     */
                    memcpy(SENDBUFFER + 14, nalu_buf + RTP_PAYLOAD_MAX_SIZE * fu_seq, last_fu_pack_size);    /* ?????????nalu??? */

                    /*
                     * 4. ??????????????????rtp???????????????
                     */
                    len_sendbuf = 12 + 2 + last_fu_pack_size;
                    send_data_to_client_list(SENDBUFFER, len_sendbuf, sock_fd);

                } /* else-if (fu_seq == 0) */
            } /* end of for (fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) */

        } /* end of else-if (nalu_len <= RTP_PAYLOAD_MAX_SIZE) */

#if 0
        if (nalu_buf) {
            free(nalu_buf);
            nalu_buf = NULL;
        }
#endif

    return 0;
} /* void *h264tortp_send(VENC_STREAM_S *pstream, char *rec_ipaddr) */

int h264naltortp(int framerate, uint8_t *nal_buf, int nalu_len, int sock_fd);
int h264naltortp(int framerate, uint8_t *nal_buf, int nalu_len, int sock_fd)
{
    send(sock_fd, nal_buf, nalu_len, 0);
    usleep(1000 * 50);
    return 0;
}

void show_usage(char *argv);
void show_usage(char *argv)
{
    fprintf(stderr, "usage: %s [OPTIONS] inputfile\n", argv);
    fprintf(stderr, "options:\n");
    fprintf(stderr, "        -h host       remote host\n");
    fprintf(stderr, "        -t|u          send via tcp(default) or udp\n");
    fprintf(stderr, "        -p port       dest_port\n");
    fprintf(stderr, "        -q            setsocktcp tcp_nodelay\n");

    exit(1);
}

int main(int argc, char **argv)
{
    char *thisarg;
    int sock_fd;
    FILE *h264file;
    uint16_t port = DEFAULT_PORT;
    char *myname = argv[0];
    char remote_host[32] = "";
    uint8_t nal_buf[MAX_NAL_BUF_SIZE];
    int len;
    int ret;

    open_debug_log("_test_tcp_send_nal.log");

    argc--;
    argv++;
    /* parse any options */
    while (argc >= 1 && **argv == '-') {
        thisarg = *argv;
        thisarg++;
        switch (*thisarg) {
        case 't':
            break;
        case 'u':
            USE_TCP = 0;
            break;
        case 'p':
            if (--argc <= 0)
                show_usage(myname);
            argv++;
            port = atoi(*argv);
            break;
        case 'h':
            if (--argc <= 0)
                show_usage(myname);
            argv++;
            strncpy(remote_host, *argv, sizeof(remote_host));
            break;
        case 'q':
            TCP_NODELAY_FLAG = 1;
            break;
        default:
            show_usage(myname);
        }
        argc--;
        argv++;
    }
    if (argc < 1)
        show_usage(myname);

    debug_print();
    h264file = fopen(*argv, "r");
    if (!h264file) {
        perror("fopen");
        exit(1);
    }

    /* init_socket(); */
    sock_fd = create_server_socket(remote_host, port);

    while (copy_nal_from_file(h264file, nal_buf, &len) != -1) {
        ret = h264naltortp_send_tcp(25, nal_buf, len, sock_fd);
    }

    close(sock_fd);
    fclose(h264file);
    close_debug_log();
    return 0;
}
