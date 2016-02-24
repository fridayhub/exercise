/*用回溯的方法解决8皇后问题的步骤为：
 *
 * 1）从第一列开始，为皇后找到安全位置，然后跳到下一列
 *
 * 2）如果在第n列出现死胡同，如果该列为第一列，棋局失败，否则后退到上一列，在进行回溯
 *
 * 3）如果在第8列上找到了安全位置，则棋局成功。
 *
 */

#include <stdio.h>
#include <stdlib.h>

static int gEightQueen[8] = {0};  //用一个长度为8的整数数组gEightQueen代表成功摆放的8个皇后，数组索引代表棋盘的列向量，而数组的值为棋盘的行向量
static int gCount = 0;

void print()
{
    int outer;
    int inner;

    for(outer = 0; outer < 8; outer++){
        for(inner = 0; inner < gEightQueen[outer]; inner++)
            printf("* ");

        printf("# ");

        for(inner = gEightQueen[outer] + 1; inner < 8; inner++)
            printf("* ");

        printf("\n");
    }

    printf("=================================\n");
}

int check_pos_valid(int loop, int value) //loop:行 value:列
{
    int index;
    int data;

    for(index = 0; index < loop; index++){
        data = gEightQueen[index]; //取出行和列 data:列  index:行

        if(value == data)  //判断同一列
            return 0;

        if((index + data) == (loop + value)) //判断斜向正方向
            return 0;
        if((index - data) == (loop - value)) //判断斜向反方向
            return 0;
    }

    return 1;
}

void eight_queen(int index)
{
    int loop;

    for(loop = 0; loop < 8; loop++){
        if(check_pos_valid(index, loop)){  //index:行 loop:列
            gEightQueen[index] = loop;

            if(7 == index){
                gCount++; 
                print();
                gEightQueen[index] = 0;
                return;
            }

            eight_queen(index + 1);
            gEightQueen[index] = 0;
        }
    }
}

int main()
{
    eight_queen(0);
    printf("total = %d\n", gCount);

    return 1;
}
