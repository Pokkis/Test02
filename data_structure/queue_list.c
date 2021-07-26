#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "queue_list.h"

//#define MAXSIZE 3  //一般场景都会指定一个最大的队列大小

bool empty_queue(queue *p_queue)
{
    //如果队列不存在直接返回队列空
    if (NULL == p_queue)
    {
        return true;
    }

    return (p_queue->head == NULL ? true : false);
}

#ifdef MAXSIZE
bool full_queue(queue *p_queue)
{
    //如果队列不存在直接返回队列满
    if (NULL == p_queue)
    {
        return true;
    }

    //如果队列为空直接返回队列满
    if (empty_queue(p_queue))
    {
        return false;
    }

    return (p_queue->node_size == MAXSIZE ? true : false);
}
#endif

int get_queue_size(queue *p_queue)
{
    //如果队列不存在直接返回
    if (NULL == p_queue)
    {
        return 0;
    }

    //如果队列为空直接返回
    if (empty_queue(p_queue))
    {
        return 0;
    }

    return p_queue->node_size;
}

int create_queue(queue *p_queue)
{
    if (NULL == p_queue)
    {
        return -1;
    }

    p_queue->head = NULL;
    p_queue->rear = NULL;
    p_queue->node_size = 0;
    return 0;
}

//入队列,以循环队列方式
int push_queue(queue *p_queue, data p_data)
{
    if (NULL == p_queue)
    {
        return -1;
    }

#ifdef MAXSIZE
    //判断队列满了直接返回
    if (full_queue(p_queue))
    {
        return -1;
    }
#endif

    node *temp = (node *)malloc(sizeof(node));
    temp->data = p_data;
    temp->next = NULL;

    if (p_queue->head == NULL)
    {
        p_queue->head = temp;
        p_queue->rear = temp;
    }
    else
    {
        p_queue->rear->next = temp;
        p_queue->rear = temp;
    }

    p_queue->node_size++;
    return 0;
}

//出队列,以循环队列方式
int pop_queue(queue *p_queue, data *p_data)
{
    //如果队列为空直接返回
    if (empty_queue(p_queue))
    {
        return -1;
    }

    node *temp = p_queue->head;
    *p_data = p_queue->head->data;
    p_queue->head = p_queue->head->next;
    free(temp);
    p_queue->node_size--;

    return 0;
}

//销毁
int destroy_queue(queue *p_queue)
{
    //如果队列为空直接返回
    if (empty_queue(p_queue))
    {
        return 0;
    }

    node *temp = NULL;
    while (--p_queue->node_size)
    {
        temp = p_queue->head;
        p_queue->head = p_queue->head->next;
        free(temp);
    }

    return 0;
}