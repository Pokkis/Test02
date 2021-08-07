#ifndef __SERVER_H_
#define __SERVER_H_
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <inttypes.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _my_server MyServer;

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef WORD
typedef unsigned short WORD;
#endif

#ifndef DWORD
typedef unsigned int DWORD;
#endif

#ifndef UINT_MAX
#define UINT_MAX 0xffffffff
#endif

#ifndef PACKED
#define PACKED		__attribute__((packed, aligned(1)))
#endif

#define MAX_CLIENT_NUM 4

typedef struct _client_session
{
	MyServer			*ourServer;
	int 			  	sock;
	int			 		bUse;
	int					bIsActive;
	struct timeval		rtcpKeepAliveTime;
	unsigned long long  session;
	char				sendBuf[4096];
	char				recvBuf[4096];
}PACKED ClientSession;  

struct _my_server
{
	int					myPort;
	int					mySocket;
	int					clientNum;
	ClientSession		client[MAX_CLIENT_NUM];	
}PACKED; 

#ifdef __cplusplus
}
#endif

#endif
