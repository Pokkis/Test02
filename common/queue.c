#include "queue.h"
#include <string.h>
#include <stdlib.h>

sequeue_data_t *CreateEmptyDataSequeue(int oneDataSize)
{
    sequeue_data_t *queue;

    queue = (sequeue_data_t *)malloc(sizeof(sequeue_data_t));

    if (queue == NULL)
        return NULL;

    queue->data = (char *)malloc(oneDataSize * MAX_DATA_SEQUEUE);
    if (NULL == queue->data)
        return NULL;

    queue->front = 0;
    queue->rear = 0;
    printf("[%s:%d]####front=%d,rear=%d####\n", __FILE__, __LINE__, queue->front, queue->rear);
    pthread_mutex_init(&queue->EnQueueLock, NULL);
    pthread_mutex_init(&queue->DeQueueLock, NULL);
    return queue;
}

void DestroyDataSequeue(sequeue_data_t *queue)
{
    if (queue == NULL)
    {
        ERR("queue=%p\n", queue);
        return;
    }

    if (queue->data)
    {
        free(queue->data);
        queue->data = NULL;
    }

    free(queue);
    queue = NULL;
}

int EmptyDataSequeue(sequeue_data_t *queue)
{
    if (NULL == queue || queue->data == NULL)
        return -1;
    //printf("[%s:%d]####queue=%p,front=%d,rear=%d####\n",__FILE__,__LINE__,queue,queue->front,queue->rear);
    return (queue->front == queue->rear ? 1 : 0);
}

int FullDataSequeue(sequeue_data_t *queue)
{
    if (NULL == queue || queue->data == NULL)
        return -1;

    return ((queue->rear + 1) % MAX_DATA_SEQUEUE == queue->front ? 1 : 0);
}

int EnDataQueue(sequeue_data_t *queue, char *inData, int oneDataSize)
{
    pthread_mutex_lock(&queue->EnQueueLock);
    if (NULL == queue || queue->data == NULL)
    {
        pthread_mutex_unlock(&queue->EnQueueLock);
        return -1;
    }

    if (1 == FullDataSequeue(queue))
    {
        pthread_mutex_unlock(&queue->EnQueueLock);
        return -1; /* full */
    }

    queue->rear = (queue->rear + 1) % MAX_DATA_SEQUEUE;

    memset(queue->data + queue->rear * oneDataSize, 0, oneDataSize);
    memcpy(queue->data + queue->rear * oneDataSize, inData, oneDataSize);

    pthread_mutex_unlock(&queue->EnQueueLock);
    return 0;
}

int DeDataQueue(sequeue_data_t *queue, char *outData, int oneDataSize)
{
    pthread_mutex_lock(&queue->DeQueueLock);
    if (NULL == queue || queue->data == NULL)
    {
        pthread_mutex_unlock(&queue->DeQueueLock);
        return -1;
    }

    if (1 == EmptyDataSequeue(queue))
    {
        pthread_mutex_unlock(&queue->DeQueueLock);
        return -1; /* empty */
    }

    queue->front = (queue->front + 1) % MAX_DATA_SEQUEUE;

    if (NULL != outData)
    {
        memcpy(outData, queue->data + queue->front * oneDataSize, oneDataSize);
    }
    pthread_mutex_unlock(&queue->DeQueueLock);

    return 0;
}

int GetSequeueSize(sequeue_data_t *queue)
{
    if (NULL == queue || queue->data == NULL)
        return -1;

    int queuesize = 0;
    queuesize = (queue->rear > queue->front) ? queue->rear - queue->front : MAX_DATA_SEQUEUE + queue->rear - queue->front;
    return queuesize;
}

int DeAllDataQueue(sequeue_data_t *queue)
{
    if (NULL == queue || queue->data == NULL)
        return -1;

    pthread_mutex_lock(&queue->EnQueueLock);
    queue->front = 0;
    queue->rear = 0;
    pthread_mutex_unlock(&queue->EnQueueLock);
    return 0;
}






