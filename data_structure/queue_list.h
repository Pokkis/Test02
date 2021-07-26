
//节点数据不存太复杂，就存一个姓名和编号
typedef struct _data
{
    char name[32];
    int num;
} data;

typedef struct _node node;
struct _node
{
    data data;
    node *next;
};

typedef struct _queue
{
    node *head;
    node *rear;
    int node_size;
} queue;

int get_queue_size(queue *p_queue);
int create_queue(queue *p_queue);
int push_queue(queue *p_queue, data _data);
int pop_queue(queue *p_queue, data *p_data);
int destroy_queue(queue *p_queue);