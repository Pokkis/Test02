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
    node *data;
    unsigned int node_size;
} queue;

int get_queue_size(queue *p_queue);
int create_queue(queue *p_queue, int size);
int push_queue(queue *p_queue, node _node);
int pop_queue(queue *p_queue, node *p_node);
int destroy_queue(queue *p_queue);
