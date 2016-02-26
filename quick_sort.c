//http://blog.csdn.net/morewindows/article/details/6684558
#include <stdio.h>

/*
 * s[] 待排序数组
 * l 基准左边
 * r 基准右边*/
int AdjustArray(int s[], int l, int r) //返回调整后基准数的位置
{
    int i = l, j = r;
    int x = s[l]; //s[1]即s[i]第一个坑 先把s[l]保存起来
    while(i < j)
    {
        //从右向左找小于x的数来填s[i]
        while(i < j && s[j] >= x)
            j--;
        if(i < j)
        {
            s[i] = s[j]; //将s[j]填到s[i]中，s[j]就形成了一个新坑
        }
        
        //从左向右找大于x的数来填s[j]
        while(i < j && s[i] < x)
            i++;
        if(i < j)
        {
            s[j] = s[i]; //将s[i]填到s[j]中，s[i]就形成了一个新坑
            j--;
        }
    }  

    //退出时，i等于j。将x填到这个坑中。
    s[i] = x;

    return i;
}

void quick_sort1(int s[], int l, int r)
{
    if(l < r)
    {
        int i = AdjustArray(s, l, r); 
        quick_sort1(s, l, i - 1); //递归调用
        quick_sort1(s, i + 1, r);
    }
}


void quick_sort(int s[], int l , int r)
{
    if(l < r)
    {
        int i = l, j = r, x = s[l];
        while( i < j)
        {
            while (i < j && s[j] >= x) //从右向左找第一个小于x的数
                j--;
            if(i < j)
                s[i++] = s[j];

            while (i < j && s[i] < x) //从左向右找第一个大于等于x的数
                i++;
            if(i < j)
                s[j--] = s[i];
        }
        s[i] = x;
        quick_sort(s, l, i - 1);
        quick_sort(s, i + 1, r);
    }
}

int main()
{
    int i = 0;
    int s[10] = {1, 434, 34, 354, 676, 83, 495, 576, 78, 8245};
    quick_sort(s, 1, 9); 
    for(i = 0; i < 10 ; i++)
    {
        printf("%d ",s[i]);
    }
    printf("\n");

    return 0;
}
