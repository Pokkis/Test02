#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "queue_linear_pointer.h"

bool empty_queue(queue *p_queue)
{
    //如果队列不存在直接返回队列空
    if (NULL == p_queue || NULL == p_queue->data)
    {
        return true;
    }

    return (p_queue->rear == p_queue->head ? true : false);
}

bool full_queue(queue *p_queue)
{
    //如果队列不存在直接返回队列满
    if (NULL == p_queue || NULL == p_queue->data)
    {
        return true;
    }

    //如果队列为空直接返回
    if (empty_queue(p_queue))
    {
        return false;
    }

    return (p_queue->rear % p_queue->node_size == p_queue->head % p_queue->node_size ? true : false);
}

int get_queue_size(queue *p_queue)
{
    //如果队列不存在直接返回
    if (NULL == p_queue || NULL == p_queue->data)
    {
        return 0;
    }

    //如果队列为空直接返回
    if (empty_queue(p_queue))
    {
        return 0;
    }

    return (p_queue->rear % p_queue->node_size > p_queue->head % p_queue->node_size ? p_queue->rear % p_queue->node_size - p_queue->head % p_queue->node_size : p_queue->node_size - p_queue->head % p_queue->node_size + p_queue->rear % p_queue->node_size);
}

int create_queue(queue *p_queue, int size)
{
    if (NULL == p_queue || 0 >= size)
    {
        return -1;
    }

    p_queue->head = 0;
    p_queue->rear = 0;
    p_queue->data = (node *)malloc(size * sizeof(node));
    p_queue->node_size = size;
    return 0;
}

//入队列,以循环队列方式
int push_queue(queue *p_queue, node _node)
{
    //判断队列满了直接返回
    if (full_queue(p_queue))
    {
        return -1;
    }

    p_queue->data[p_queue->rear % p_queue->node_size] = _node;
    p_queue->rear++;
    return 0;
}

//出队列,以循环队列方式
int pop_queue(queue *p_queue, node *p_node)
{
    //如果队列为空直接返回
    if (empty_queue(p_queue))
    {
        return -1;
    }

    *p_node = p_queue->data[p_queue->head % p_queue->node_size];
    p_queue->head++;

    return 0;
}

//销毁
int destroy_queue(queue *p_queue)
{
    //如果队列不存在直接返回
    if (NULL == p_queue || NULL == p_queue->data)
    {
        return -1;
    }

    p_queue->head = 0;
    p_queue->rear = 0;
    p_queue->node_size = 0;
    free(p_queue->data);

    return 0;
}