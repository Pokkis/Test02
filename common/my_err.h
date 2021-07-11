#ifndef _MY_ERR_H_
#define _MY_ERR_H_

#ifdef __cplusplus  
extern "C"{
#endif
#include <stdio.h>

#ifndef MERGEFD
#define	MERGEFD(fd,set)	\
	do {FD_SET(fd, set); if (fd > maxfd) maxfd = fd; } while (0)
#endif

#ifndef DBG
#define DBG(fmt, args...) do { \
				 fprintf(stdout, "\033[m""[-DBG-] [%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);	 \
			 } while(0)
#endif
			
#ifndef SUCCESS_TRACE
#define SUCCESS_TRACE(fmt, args...) do { \
				 fprintf(stdout, "\033[1;32m""[SUCCESS_TRACE!] [%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);	\
			 } while(0)
#endif
			
#ifndef WARNING_TRACE
#define WARNING_TRACE(fmt, args...) do { \
				 fprintf(stdout, "\033[1;33m""[WARNING_TRACE!] [%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);	\
			 } while(0)
#endif
			
#ifndef BLUE_TRACE
#define BLUE_TRACE(fmt, args...) do { \
				 fprintf(stdout, "\033[1;34m""[TRACE!] [%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);	\
			 } while(0)
#endif
			
#ifndef MAGENTA_TRACE
#define MAGENTA_TRACE(fmt, args...) do { \
				 fprintf(stdout, "\033[1;35m""[TRACE!] [%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);	\
			 } while(0)
#endif
			
#ifndef CYAN_TRACE
#define CYAN_TRACE(fmt, args...) do { \
				 fprintf(stdout, "\033[1;36m""[TRACE!] [%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);	\
			 } while(0)
#endif
			
#ifndef ERR
#define ERR(fmt, args...) do { \
				 fprintf(stderr, "\033[1;31m""[ERR!] [%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);	\
			 } while(0)
#endif

 #define    SUCCESS                     0

//通用错误 -1～-50
 #define    GENNALRA_NULL_POINTER       SUCCESS-1


//文件错误

#ifdef __cplusplus
}
#endif
#endif