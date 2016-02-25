#include <stdio.h>
#include <assert.h>

static int my_atoi(const char* str)
{
    int result = 0;
    int sign = 0;
    assert(str != NULL);
    
    //process whitespace characters
    //while(*str == ' ' || *str == '\t' || *str == '\n')
    while(!(*str >= '0' && *str <= '9') && (*str != '-') && (*str != '+'))
        ++str;

    //process sign character
    if(*str == '-')
    {
        sign = 1;
        ++str;
    }else if(*str == '+')
    {
        ++str;
    }

    //process numbers
    while(*str >= '0' && *str <= '9')
    {
        result = result*10 + (*str - '0'); //字符‘0’的ASCLL值是48，而字符‘1’是49，所以str[0]-'0'相当于49-48=1
        ++str;
    }

    if(sign == 1)
        return -result;
    else
        return result;
}

/*
 * value:欲转换的数据
 * string:目标字符串的地址
 * radix:转换后的进制，可以是10进制 16进制*/
char *my_itoa(int val, char *buf, unsigned radix)
{
    char *p;
    char *firstdig;
    char temp;
    unsigned digval;
    p = buf;
    if(val < 0)
    {
        *p++ = '-';
        val = (unsigned long)(-(long)val);
    }
    firstdig = p;
    do{
        digval = (unsigned)(val % radix);
        val /= radix;

        if(digval > 9)
            *p++ = (char)(digval - 10 + 'a'); //16进制
        else
            *p++ = (char)(digval + '0');
    }while(val > 0);

    *p-- = '\0';
    do{
        temp = *p;
        *p = *firstdig;
        *firstdig = temp;
        --p;
        ++firstdig;
    }while(firstdig < p);

    return buf;
}

int main()
{
    char p[] = "asf-12323aaa948";
    int chang = my_atoi(p);
    char buf[100];
    int num = 2345403;
    printf("%d\n\n", chang);
    
    printf("%s\n", my_itoa(num, buf, 10));
}
