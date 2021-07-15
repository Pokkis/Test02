#include "file_parse.h"
#include "../common/my_err.h"
#include <stdio.h>
#include <stdbool.h>


enum NAL_UNIT_TYPE
{
    NAL_SLICE               = 1,
    NAL_DPA                 = 2,
    NAL_DPB                 = 3,
    NAL_DPC                 = 4,
    NAL_IDR_SLICE           = 5,
    NAL_SEI                 = 6,
    NAL_SPS                 = 7,
    NAL_PPS                 = 8,
    NAL_AUD                 = 9,
    NAL_END_SEQUENCE        = 10,
    NAL_END_STREAM          = 11,
    NAL_FILLER_DATA         = 12,
    NAL_SPS_EXT             = 13,
    NAL_AUXILIARY_SLICE     = 19,
    NAL_FF_IGNORE           = 0xff0f001,
};

typedef struct _nal_str
{
    int  nal_type;
    char *str;
}nal_str;

static const nal_str  g_nal_str[] = 
{
    {NAL_SLICE, "NAL_SLICE"},
    {NAL_DPA, "NAL_DPA"},
    {NAL_DPB, "NAL_DPB"},
    {NAL_DPC, "NAL_DPC"},
    {NAL_IDR_SLICE, "NAL_IDR_SLICE"},
    {NAL_SEI, "NAL_SEI"},
    {NAL_SPS, "NAL_SPS"},
    {NAL_PPS, "NAL_PPS"},
    {NAL_AUD, "NAL_AUD"},
    {NAL_END_SEQUENCE, "NAL_END_SEQUENCE"},
    {NAL_END_STREAM, "NAL_END_STREAM"},
    {NAL_FILLER_DATA, "NAL_FILLER_DATA"},
    {NAL_SPS_EXT, "NAL_SPS_EXT"},
    {NAL_AUXILIARY_SLICE, "NAL_AUXILIARY_SLICE"},
    {NAL_FF_IGNORE, "NAL_FF_IGNORE"},
};

int print_nal_type(int nal_type)
{
    static int I = 0;
    static int P = 0;
    int i = 0;
    for(i = 0; i < sizeof(g_nal_str) / sizeof(g_nal_str[0]); i++)
    {
        if(nal_type == g_nal_str[i].nal_type)
        {
            if(nal_type == NAL_IDR_SLICE)
            {
                I++;
            }
            else if(nal_type == NAL_SLICE)
            {
                P++;
            }
            BLUE_TRACE("type:%d %s I:%d P:%d\n", g_nal_str[i].nal_type, g_nal_str[i].str, I, P);
        }
    }
   return 0; 
}

static bool find_nal_type(char *buff, int n_read_len)
{
    if(NULL == buff || n_read_len < 4)
    {
        return false;
    }

    if(buff[0] == 0x00 && buff[1] == 0x00 && 
        buff[2] == 0x00 && buff[3] == 0x01)
    {
        return true;
    }

    return false;
}

char* find_nal_start_code(char *buff, int n_read_len)
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


int parse_h264_file(char *buff, int n_read_len, int *n_parse_len)
{
    if(NULL == buff || NULL == n_parse_len)
    {
        ERR("parameter err\n");
        return -1;
    }

    char *p_buff = buff;
    char *p_parse_buff = buff;
    *n_parse_len = 0;
    int parse_step = 1;
    int nal_type = 0;

    while(n_read_len)
    {
        parse_step = 1;
        if(find_nal_type(p_buff, n_read_len))
        {
            nal_type = *(p_buff+4) & 0x1f;
            //nal_type = (*(p_buff+4) & 0xf8) >> 3;
            BLUE_TRACE("nal_type:%d nal-heard:%d\n", nal_type, *(p_buff+4));
            print_nal_type(nal_type);
            parse_step = 4;
            p_parse_buff = p_buff + 4;
        }
        p_buff += parse_step;
        n_read_len -= parse_step;
    }
    *n_parse_len = p_parse_buff - buff;

    return 0;
}

int copy_nal_from_file(FILE *fp, char *p_buff, int buff_len, int *read_len)
{
    if(NULL == fg || NULL == p_buff || NULL == read_len)
    {
        ERR("NULL pointer\n");
        return -1;
    }

    static char temp_buff[1024] = { 0 };
    static int temp_last = 0;
    *read_len = 0;
    //记录start_code次数
    int start_code_count = 0;

    int n = 0;
    char *p_tmp = NULL; 
    int copy_len = 0;
    while((fread(temp_buff+temp_last, 1, 1024 - temp_last, fp)) > 0)
    {
        if(start_code_count == 0)
        {
            p_tmp = find_nal_start_code(temp_buff, 1024);
            if(p_tmp)
            {
                char *p_start = p_tmp; //记录找到start_code buff的位置
                p_tmp = find_nal_start_code(p_tmp+4, 1024 - (p_start - temp_buff));
                if(p_tmp)
                {
                    if(buff_len > (p_tmp - p_start))
                    {
                        memcpy(p_buff+*read_len, p_start, p_tmp - p_start)
                    }
                    else
                    {
                        *read_len = 0;
                        return -1;
                    }
                    
                }
            }
        }
        else if(start_code_count == 1)
        {
            p_tmp = find_nal_start_code(temp_buff, 1024);
        }
        
    }

}
