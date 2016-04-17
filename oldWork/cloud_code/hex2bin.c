#include <stdio.h>
#include <stdlib.h>

void main(int argc, char **argv)
{
    char *c;
    int a[16][4]={0};
    int i=0,j;
    //scanf("%s",c);---->>以字符串形式输入
    c = (char *)malloc(10);
    c = argv[1];

    while(c[i])   //--------->>>把字符串的每一位还原为数字
    //while(i < 4)   //--------->>>把字符串的每一位还原为数字
    {
        if(c[i]>='0'&&c[i]<='9')
            c[i]=c[i++]-48;
        else if(c[i]>='A'&&c[i]<='Z')
            c[i]=c[i++]-55;
        else if(c[i]>='a'&&c[i]<='z')
            c[i]=c[i++]-87;
        else
        {
            puts("error\n");
            return; 
        }
    }
    i=0;
    while(i < 4)   //---》》》每一位分解为四位，注意输出顺序就可以了
    {
        for(j=3;j>=0;j--)
        { 
            a[i][j]=c[i]%2;
            c[i]/=2;
        }
        for(j=0;j<4;j++)
            printf("%d",a[i][j]);
        i++;
    }
    printf("\n");
}
