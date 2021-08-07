#include <stdio.h>
#include "queue.h"

typedef struct _node
{
    char name[32];
    int num;
} node;


int main()
{
    int cmd = 0;
    sequeue_data_t *my_queue = NULL;
    while(1)
    {
        printf("请输入你要操作的类型\n");
        printf("1、创建     2、入队     3、出队     4、销毁     5、退出\n");
        scanf("%d", &cmd);
        node n = { 0 };
        switch(cmd)
        {
            case 1:
                printf("创建成功\n");
                my_queue = CreateEmptyDataSequeue(sizeof(node));
                break;
            case 2:
                printf("请输入：姓名 编号\n");
                scanf("%s %d", n.name, &n.num);
                EnDataQueue(my_queue, (char*)&n, sizeof(n));
                break;
            case 3:
                DeDataQueue(my_queue, (char*)&n, sizeof(n));
                printf("当前姓名%s 编号%d\n", n.name, n.num);
                break;
            case 4:
                //destroy_queue(&my_queue);
                break;
            case 5:
                printf("退出\n");
                break;
            default:
                printf("操作有误\n");
                break;
        }

        printf("当前队列大小：%d\n", GetSequeueSize(my_queue));

        if(5 == cmd)
        {
            break;
        }
    }
    return 0;
}