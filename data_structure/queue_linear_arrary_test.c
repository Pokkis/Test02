#include <stdio.h>
#include "queue_linear_arrary.h"

int main()
{
    int cmd = 0;
    queue my_queue = { 0 };
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
                break;
            case 2:
                printf("请输入：姓名 编号\n");
                scanf("%s %d", n.name, &n.num);
                push_queue(&my_queue, n);
                break;
            case 3:
                pop_queue(&my_queue, &n);
                printf("当前姓名%s 编号%d\n", n.name, n.num);
                break;
            case 4:
                destroy_queue(&my_queue);
                break;
            case 5:
                printf("退出\n");
                break;
            default:
                printf("操作有误\n");
        }

        printf("当前队列大小：%d\n", get_queue_size(&my_queue));

        if(5 == cmd)
        {
            break;
        }
    }
    return 0;
}