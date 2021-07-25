#define MAXSIZE 3 //需要在一开始就指明大小

//节点数据不存太复杂，就存一个姓名和编号
typedef struct _node
{
    char name[32];
    int num;
} node;

typedef struct _queue
{
    unsigned int head;
    unsigned int rear;
    node data[MAXSIZE];
} queue;

int get_queue_size(queue *p_queue);
int push_queue(queue *p_queue, node _node);
int pop_queue(queue *p_queue, node *p_node);
int destroy_queue(queue *p_queue);
