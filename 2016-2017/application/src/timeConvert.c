#include "include.h"

int k = 0;
char var[25] = {0};
unsigned char fromInt[20];

void itoas (unsigned int n, unsigned char *s)
{
    int i, j, sign;

    if((sign = n) < 0)//记录符号
        n =- n;//使n成为正数
    i = 0;
    do{
        s[i++] = n % 10 + '0';//取下一个数字
    }while ((n /= 10) > 0);//删除该数字

    if(sign < 0)
        s[i++] = '-';
    //s[i]= '\0';
    i--;
    for(j = i; j >= 0; j--)//生成的数字是逆序的，所以要逆序输出
    {
        var[k] = s[j];
        printf("%c", var[k]);
        k++;
    }
}

//get currnet time
long int GetCurrentTime(unsigned int *CurrentTime)
{
    int i = 0;
    int flag = 1;

    for(i = 0; i < 7; i++)
    {

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
        else if(flag == 11)
        {
            var[k] = ':';
            k++;
            i--;
        }
        else
        {
            itoas(CurrentTime[i], fromInt);
        }

        flag++;
    } 
    var[k] = '\0';
    return 1;
}

long int getConvertTime(char *occurTime)
{
    unsigned int CurrentTime[7] = {0};
    int i = 0;
    CurrentTime[0] = 20;

    struct tm tm_time;
    long int unixtime;

    memset(var, '0', sizeof(var));
    k = 0;
    for(i = 0; i < 6; i++)
    {
        CurrentTime[i+1] = occurTime[i];
    }

    GetCurrentTime(CurrentTime);
    for(i = 0; i < k; i++)
    {
        printf("%c", var[i]);
    }

    strptime(var, "%Y-%m-%d %H:%M:%S", &tm_time);
    unixtime = mktime(&tm_time);

    return unixtime;

}

int SetRTC_Time(char *ct)
{
	char cmdline[50]={0};
	#if 0
	FILE *stream;
	char buf[50]={0};
	sprintf(cmdline , "date -d \"1970-01-01 UTC %ld seconds\"",current);
	stream = popen(cmdline , "r");
	fread(buf , sizeof(char) , sizeof(buf) , stream);
	pclose(stream);
	printf("time is %s",buf);
	memset(cmdline , 0 , 50);
	#endif
	sprintf(cmdline , "date -s \"%s\"",ct);
	system(cmdline);
	
}
