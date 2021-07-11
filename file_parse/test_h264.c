#include <stdio.h>
#include <string.h>
#include <errno.h>
//#include "../file_parse/file_parse.h"
#include "file_parse.h"
#include "../common/my_err.h"

#define H264_FILE_NAME   "../resource/128x128.264"
#define MAX_BUFF_LEN      1024

int main()
{
    FILE *p_fp; //要读取的h264文件
    int n_parse_remain_len = 0; //解析后剩下的长度
    int n_read_len = 0; //当前读取的文件内容长度
    char buff[MAX_BUFF_LEN] = { 0 };  //缓冲区buff

    p_fp = fopen(H264_FILE_NAME, "rb");
    if(NULL == p_fp)
    {
        ERR("open file fail! %d\n", errno);
        return -1;
    }

    while((n_read_len = fread(buff+n_parse_remain_len, 1, MAX_BUFF_LEN - n_parse_remain_len, p_fp)) > 0)
    {
        parse_h264_file(buff, n_read_len, &n_parse_remain_len);
        if(n_parse_remain_len)
        {
            memmove(buff, buff + n_read_len - n_parse_remain_len, n_parse_remain_len);
        }
        //printf("n_read_len:%d\n", n_read_len);
    }

    return 0;
}