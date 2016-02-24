#include <stdio.h>
#include <stdlib.h>

//冒泡排序 升序排列
void bubblesort(int *a, int length)
{
    int temp;
    int i, j;
    
    for(i = 0; i < length; i++)
    {
        for(j = i+1; j < length; j++)
        {
            if(a[i] > a[j])
            {
                temp = a[i];
                a[i] = a[j];
                a[j] = temp;
            }
        }
    }

}

//n 数组长度  升序排列
void insert_sort(int *sort, int n)
{
    int i, j;
    int key;
    for(j = 1; j < n; j++)
    {
        key = sort[j];

        //将sort[j]插入到已经排序好的数组sort[0...j-1];
        i = j - 1;
        while(i > -1 && sort[i] > key)
        {
            sort[i + 1] = sort[i];
            i--;
        }
        sort[i + 1] = key;
    }
}

int main(void)
{
    int i;
    int a[10] = {11, 2, 3, 40, 5, 6, 7, 8, 9, 10};
    //bubblesort(a, 10);
    for(i = 0; i < 10; i++)
        printf("%d ,", a[i]);

    printf("\n");

    insert_sort(a, 10);

    for(i = 0; i < 10; i++)
        printf("%d ,", a[i]);

    printf("\n");
    
    return 0;
}
