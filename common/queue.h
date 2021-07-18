#ifndef _QUEUE_H_
#define _QUEUE_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include <pthread.h>

#define MAX_DATA_SEQUEUE 100
    typedef struct _queue_node
    {
        void *data;
        int size;
        queue_node *front;
        queue_node *next;
    } queue_node;

    //公共的数据队列接口
    typedef struct
    {
        int front;
        int rear;
        pthread_mutex_t EnQueueLock; //进队列互斥锁
        pthread_mutex_t DeQueueLock; //出队列互斥锁
        char *data;
    } sequeue_data_t;

    sequeue_data_t *CreateEmptyDataSequeue(int oneDataSize);
    void DestroyDataSequeue(sequeue_data_t *queue);
    int EmptyDataSequeue(sequeue_data_t *queue);
    int FullDataSequeue(sequeue_data_t *queue);
    int EnDataQueue(sequeue_data_t *queue, char *inData, int oneDataSize);
    int DeDataQueue(sequeue_data_t *queue, char *outData, int oneDataSize);
    int DeAllDataQueue(sequeue_data_t *queue);
    int GetSequeueSize(sequeue_data_t *queue);

#ifdef __cplusplus
}
#endif
#endif