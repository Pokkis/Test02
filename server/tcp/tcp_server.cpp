#include "../../common/commonsocket.h"
#include "../include/server.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PORT 554

MyServer myServer;

//#define random(x) (rand()%(x))

static int our_random()
{
	return random();
}

void our_srandom(DWORD x)
{
	srandom(x);
}

long long our_random64(void)
{
	long long i;
	i = our_random();
	i <<= 32;
	i |= our_random();
	return i;
}

static void incomingConnectionHandler(void * instance, int Mask)
{	
	CYAN_TRACE("incomingConnectionHandler\n");
	MyServer *serverHand = (MyServer *)instance;
	if(NULL == serverHand)
	{
		ERR("serverHand=%p\n",serverHand);
		return;
	}

	ClientSession * tempClientSession = NULL;
	int clientSocket;
	struct timeval timeNow;

	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	clientSocket = accept(serverHand->mySocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

	if (clientSocket < 0)
	{
		return;
	}


	DBG("recv remot rtsp client %s\n",comm_socket_getPeerIp(clientSocket));
	
	comm_socket_nonblock(clientSocket, 1);
	//comm_setSendBufferTo(clientSocket, 512 * 1024);

	int i = 0;
	for(i = 0; i < MAX_CLIENT_NUM; i++)
	{
		tempClientSession = &(serverHand->client[i]);
		if(tempClientSession->bUse == 0)
			break;
		continue;
	}

	if(i >= MAX_CLIENT_NUM)
	{
		ERR("more than 4 channels RTSP stream,disconnect\n");
		comm_socket_close(clientSocket);
		return;
	}

	gettimeofday(&timeNow, NULL);
	our_srandom(timeNow.tv_sec*1000 + timeNow.tv_usec/1000);
	memset(tempClientSession,0,sizeof(ClientSession));
	serverHand->clientNum++;

	DBG("recv remot rtsp client %s, clientNum = %d\n",comm_socket_getPeerIp(clientSocket), serverHand->clientNum);
	tempClientSession->sock = clientSocket;
	tempClientSession->session = our_random64();
	tempClientSession->bUse = 1;
	tempClientSession->bIsActive = 1;
	tempClientSession->ourServer = serverHand;
	memset(tempClientSession->sendBuf, 0, sizeof(tempClientSession->sendBuf));
	memset(tempClientSession->recvBuf, 0, sizeof(tempClientSession->recvBuf));
}

void incomingConnectionHandlerClient(void * instance, int Mask)
{
	ClientSession *clientSession = (ClientSession *)instance;
	int  bytesRead,i;
	char *ptr = clientSession->recvBuf;

	struct timeval timeNow;
	int  ret = 0;

	memset(ptr, 0, sizeof(clientSession->recvBuf));
	memset(clientSession->sendBuf, 0, sizeof(clientSession->sendBuf));
	bytesRead = recv(clientSession->sock, ptr, 4096, 0);
	if(bytesRead <= 0)
	{
		if(bytesRead == -1 &&errno != EINTR && errno != EAGAIN)
		{
			clientSession->bIsActive = -1;
			BLUE_TRACE("clientSession->bIsActive=%d\n",clientSession->bIsActive);
		}
		else if(bytesRead == 0)
		{
			clientSession->bIsActive = -1;
			BLUE_TRACE("clientSession->bIsActive=%d\n",clientSession->bIsActive);
		}
		return;
	}
	
	gettimeofday(&timeNow,NULL);
	clientSession->rtcpKeepAliveTime = timeNow;

	//for(i = 0;i < bytesRead; i++)
	{
		//DBG("%x",ptr[i]);
	}
	DBG("ptr len:%d ptr:%s\n", bytesRead, ptr);
	sprintf(clientSession->sendBuf, "yse,%s", ptr);
	MAGENTA_TRACE("sock=%d,sendBuf=%s\n",clientSession->sock,clientSession->sendBuf);
	ret = comm_tcp_write(clientSession->sock,clientSession->sendBuf, strlen(clientSession->sendBuf), 5);

	if(ret <= 0&&errno != EINTR && errno != EAGAIN)
	{
		clientSession->bIsActive = -1;
		BLUE_TRACE("clientSession->bIsActive=%d\n",clientSession->bIsActive);
		return;
	}
	
}

void DestroyClientSession(ClientSession *clientSession)
{
	int				bMinStream = 0,nCountMultiSessNum = 0;
	MyServer		*pRtspServer = &myServer;

	if(pRtspServer== NULL)
		return;

	WARNING_TRACE("DestroyClientSession clientNum:%d\n", pRtspServer->clientNum);

	if(NULL == clientSession)
		return;

	if(clientSession->bUse == 0)
	{
		return;
	}

	if(clientSession->sock > 0)
	{
		comm_socket_close(clientSession->sock);
	}



	memset(clientSession, 0, sizeof(ClientSession));
	pRtspServer->clientNum--;

	if(pRtspServer->clientNum < 0)
		pRtspServer->clientNum = 0;

	DBG("DestroyClientSession client-- clientNum:%d\n", pRtspServer->clientNum);
}

void DestroyServer(int line, MyServer *serverHand)
{
	int	i;
	ClientSession * tempClientSession;
	MAGENTA_TRACE("DestroyServer begin line=%d\n",line);
	if(serverHand == NULL)
		return;
	if(serverHand->mySocket > 0)
	{
		serverHand->mySocket = 0;
	}
	for(i = 0; i < MAX_CLIENT_NUM; i++)
	{
		tempClientSession = &(serverHand->client[i]);
		if(tempClientSession->bUse == 1)
		{
			DestroyClientSession(tempClientSession);
			BLUE_TRACE("tempClientSession->bIsActive=%d\n",tempClientSession->bIsActive);
		}
	}
	DBG("DestroyRtspServer end \n");
}


int main()
{	
	memset(&myServer , 0, sizeof(MyServer));
	memset(myServer.client, 0, sizeof(ClientSession) * MAX_CLIENT_NUM);	
	myServer.myPort = PORT;
	myServer.mySocket = comm_isocket_creat(AF_INET, INTF_TCP, myServer.myPort);
	
	if( listen( myServer.mySocket, 10 ) != 0 )
	{
		ERR("socket listen");
		close( myServer.mySocket );
		return -1;
	}
	struct timeval tv;
	fd_set readSet;
	int maxfd = 0;
	int selectResult;
	int i;
	//system("netstat -an");
	while(1)
	{
		tv.tv_sec  = 0;
		tv.tv_usec = 10000;
		maxfd = 0;
		FD_ZERO(&readSet);
		MERGEFD(myServer.mySocket,&readSet);
		for(i = 0; i < MAX_CLIENT_NUM; ++i)
		{
			if(myServer.client[i].bUse)
			{
				MERGEFD(myServer.client[i].sock,&readSet);
			}
		}
		selectResult = select(maxfd+1, &readSet, NULL, NULL,&tv);
		if (selectResult > 0)
		{
			if (FD_ISSET(myServer.mySocket,&readSet))
			{
				incomingConnectionHandler(&myServer,0);
			}
			for(i = 0;i<MAX_CLIENT_NUM;++i)
			{
				if(myServer.client[i].bUse)
				{
					if(FD_ISSET(myServer.client[i].sock,&readSet))
						incomingConnectionHandlerClient(&myServer.client[i], 0);
				}
			}
			ClientSession *tempClientSession = NULL;
			for(i = 0; i < MAX_CLIENT_NUM; i++)
			{
				tempClientSession = &(myServer.client[i]);
				if(tempClientSession->bIsActive == -1)
				{
					DestroyClientSession(tempClientSession);
				}
			}
			
		}
	}
	DestroyServer(__LINE__, &myServer);
	return 0;
}
