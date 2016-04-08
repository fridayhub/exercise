/*将ASCII转换成整int，再转成字符串，通过时间函数转成时间戳*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

int k = 0;
char var[20];
unsigned char fromInt[20];

void itoa (int n,char *s)
{
    int i,j,sign;

    if((sign=n)<0)//记录符号
        n=-n;//使n成为正数
    i=0;
    do{
        s[i++]=n%10+'0';//取下一个数字
    }while ((n/=10)>0);//删除该数字

    if(sign<0)
        s[i++]='-';
    //s[i]= '\0';
    i--;
    for(j=i;j>=0;j--)//生成的数字是逆序的，所以要逆序输出
    {
        var[k] = s[j];
        printf("%c", var[k]);
        k++;
    }
}

//get currnet time
//时间从数据最后的校验和字节往前读取六个字节
int GetCurrentTime(char *RxBuffer, unsigned int *CurrentTime)
{
    int i = 0;
    int len = 0;
    int flag = 1;
    while(RxBuffer[i] != 0x23)
    {
        len++;
        i++;
    }
    if(len < 30) //应用数据单元为空
        return -1;
 //   printf("%d\n", len);
    len--;
    //len--;
    for(i = 1; i < 7; i++)
    {
        CurrentTime[i] = RxBuffer[len - i];
        //printf("get num is %d\n", CurrentTime[i]);
        printf("flag %d\n", flag);

        if(flag == 3)
        {
            var[k] = '-';
            k++;
            i--;
        }
        else if (flag == 5)
        {
            var[k] = '-';
            k++;
            i--;
        }
        else if (flag == 7)
        {
            var[k] = ' '; 
            k++;
            i--;
        }
        else if(flag == 9)
        {
            var[k] = ':';
            k++;
            i--;
        }
        else
        {
            itoa(CurrentTime[i-1], fromInt);
        }

        flag++;
    } 
    var[k] = ':';   //打印最后一个" : "
    k++;
    itoa(CurrentTime[6], fromInt);
    return 1;
}

long int  getConvertTime(unsigned char *TxBuffer)
{
   unsigned int CurrentTime[8];
    int i = 0, j = 0;
    CurrentTime[0] = 20;
    unsigned int times[20];

    struct tm tm_time;
    long int unixtime;

    GetCurrentTime(TxBuffer, CurrentTime);
    

    printf("\n");
    for(i = 0; i < 20; i++)
    {
        printf("%c", var[i]);
    }

    strptime(var, "%Y-%m-%d %H:%M:%S", &tm_time);
    printf("\n");
    unixtime = mktime(&tm_time);
    printf("unixtime : %ld\n", unixtime);

    return unixtime;

}
