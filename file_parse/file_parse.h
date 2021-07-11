#ifndef _FILE_PARSE_H_
#define _FILE_PARSE_H_

#ifdef __cplusplus  
extern "C"{
#endif

 int parse_h264_file(char *buff, int n_read_len, int *n_parse_len);

#ifdef __cplusplus
}
#endif
#endif