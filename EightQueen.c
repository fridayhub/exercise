#include <stdio.h>
#include <stdlib.h>

static int sum = 0;
const int max = 8;/* max为棋盘最大坐标 */
/* 输出所有皇后的坐标 */
void show(int result[])
{
    int i;
    for(i = 0; i < max; i++)
    {
        printf("(%d,%d) ", i, result[i]);
    }
    printf("\n");
    sum++;
}

/* 检查当前列能否放置皇后 */
int check(int result[], int x)
{
    int i;
    for(i = 0; i < x; i++)/* 检查横排和对角线上是否可以放置皇后 */
    {
        if(result[i] == result[x] 
            || abs(result[i] - result[x]) == (x - i))
        {
            return 0;
        }
    }
    return 1;
}

/* 回溯尝试皇后位置,n为横坐标 */
void Queen(int result[], int x)
{
    int i;
    if(x == max)
    {
        show(result);/* 如果全部摆好，则输出所有皇后的坐标 */
        return;
    }
    // 新的一行里面，皇后可以在任何位置：0->max，***
    // 所以i=0->max，而不是i=n->max ***
    for(i = 0; i < max; i++)
    {
        result[x] = i;/* 将皇后摆到当前循环到的位置 */
        if(check(result, x))
        {
            Queen(result, x + 1);/* 否则继续摆放下一个皇后 */
        }
    }
}

int main()
{
    int result[max];
    Queen(result, 0);/* 从横坐标为0开始依次尝试 */
    printf("total: %d enums.\n", sum);
    return 0;
}

