/*使用方法:
 * 此处第一块芯片指板子正面的芯片 :U502
 * 第二块芯片指板子背面芯片：U501
 * 首先用gcc编译文件，
 * 运行方式：./编译后的文件名  参数1  参数2  参数3
 *
 * 对于操作一个位：
 * 参数1：
 * 0： 读第一块芯片一个位
 * 1： 写第一块芯片一个位
 * 8： 设置第一块芯片一个位为输出
 * 3： 设置第一块芯片一个位为输入
 * 6： 读第二块芯片一个位
 * 7： 写第二块芯片一个位
 * 9： 设置第二块芯片一个位为输出
 * 10：设置第二块芯片一个位为输入
 * 
 * 参数2：
 * 芯片的具体第几个GPIO脚，范围：0-15
 *
 * 参数3:
 * 参数1选择为写入的时候，此参数表示写入的具体值
 *
 * ------------------------------------------------
 *
 * 对于操作一个字节：
 * 参数1：
 * 4： 读取第一块一个字节
 * 5： 写入第一块一个字节
 * 11：读取第二块一个字节
 * 12：写入第二块一个字节
 * 15: 设置第一块芯片8位为输出
 * 16: 设置第二块芯片8位为输出
 * 17: 设置第一块芯片8位为输入
 * 18: 设置第二块芯片8位为输入
 * 
 * 参数2:
 * 参数1为 4 或者 5 时：
 * 0:第一个字节（0-7)bit
 * 1:第二个字节（8-15)bit
 * 参数1为 11 或者 12 时：
 * 2:第一个字节（0-7)bit
 * 3:第二个字节（8-15)bit
 *
 * 参数3：
 * 当选择写入或者设置为输出的时候，参数三位设置的具体值的16进制表示形式，如：5f
 *
 *------------------------------------------------------
 *对于16位的操作：
 *参数1：
 *13 ： 读取一个16位数字
 *14 ： 写入一个16位数字
 *19 ： 设置一个16位为输出
 *20 ： 设置一个16位为输入
 *
 * 参数2：
 * 0：第一块芯片
 * 1：第二块芯片

 * 参数3：
 * 对于写操作或者设置为输出的时候，第三个参数为将要写入的数的16进制表现形式，
 * 如：需要写入16位都为1，则此参数为：FFFF
 * 
 * 例:
 * ./filename 11 2 表示读取第二块芯片的第一个字节
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

struct control_info{
	int off;   //第几个引脚
	int val;   //引脚值
	int byte_data[8];
	int d_byte_data[16];
};

struct control_info ctl_info;
int num = 0;

void print_usage(char *file)
{
	printf("cmd:  \nIOC_PCA1_RD  0 \nIOC_PCA1_WR  1 \nIOC_PCA1_SET_OUT  8 \nIOC_PCA1_SET_IN  3  \nIOC_PCA_BYTE_RD_1  4  \nIOC_PCA_BYTE_WR_1  5  \nIOC_PCA2_RD  6 \nIOC_PCA2_WR  7 \nIOC_PCA2_SET_OUT  9 \nIOC_PCA2_SET_IN  10 \nIOC_PCA_BYTE_RD_2  11 \nIOC_PCA_BYTE_WR_2  12 \nIOC_PCA_DBYTE_RD 13  \nIOC_PCA_DBYTE_WD 14  \nIOC_SET_B_OUT_1  15  \nIOC_SET_B_OUT_2 16  \nIOC_SET_B_IN_1  17  \nIOC_SET_B_IN_2  18  \nIOC_SET_DB_OUT  19  \nIOC_SET_DB_IN  20  \n");
	printf("Usage: %s cmd ping_number value\n", file);
	printf("Example: %s 1 1 1\n", file);
	printf("That means set the values 1 of chip1's number 1\n");
}

void hex2binary(char *hexs)
{
    char *c;
    int a[16][4]={0};
    int i = 0,j;
    int k = 0;
    int n = 0;
    int size = 0;
    //scanf("%s",c);---->>以字符串形式输入
    c = (char *)malloc(10);
    c = hexs;
    int bytedata[8]; //存放临时值
    int doudata[16]; //存放临时值

    //判断数组大小：
    if(num == 14 || num == 19)
    {
    	size = 4;
    }
    else if( num == 5 || num == 12 || num == 15 || num == 16)
    {
   	size = 2; 
    }
    //printf("size is %d\n", size);

    while(i < size)   //--------->>>把字符串的每一位还原为数字
    {
        if(c[i] >= '0' && c[i] <= '9')
            c[i]=c[i++]-48;
        else if(c[i] >= 'A' && c[i] <= 'Z')
            c[i] = c[i++] - 55;
        else if(c[i] >= 'a' && c[i] <= 'z')
            c[i] = c[i++] - 87;
        else
        {
            puts("error\n");
            return; 
        }
    }
    i=0;
    
    //for(n = 0; n < size ; n++)
    while( i < size )   //---》》》每一位分解为四位，注意输出顺序就可以了
    {
        for(j=3;j>=0;j--)
        { 
            a[i][j]=c[i]%2;
            c[i]/=2;
        }
        for(j=0;j<4;j++)
	{
		//    printf("%d",a[i][j]);

		//if( num == 5 || num == 12 || num == 15 || num == 16 )  //判断是8位还是16位操作，然后保存到ctl_info 结构体相应的数组中
		if(2 == size)
		{
			bytedata[k] = a[i][j];
		}else if(4 == size)      //(num == 14 || num == 19)
		{
			doudata[k] = a[i][j];
		}
		k++;
	}
        i++;
    }

    //if( num == 5 || num == 12 || num == 15 || num == 16)
    if(2 == size)
    {
	    printf("byte_data\n");
	    //while(ctl_info.byte_data[i])
	    for(i = 0; i < 8; i++)
	    {
		    ctl_info.byte_data[i] = bytedata[7-i];
		    printf("%d", ctl_info.byte_data[i]);
	    }
    }else if(4 == size)           //( num == 14 || num == 19)
    {
	    printf("d_byte_data\n");
	   // while(ctl_info.d_byte_data[i])
	   for(i = 0; i < 16; i++)
	    {
		    ctl_info.d_byte_data[i] = doudata[15-i];
		    printf("%d", ctl_info.d_byte_data[i]);
	    }
    }
    printf("\n");

}

int main(int argc, char **argv)
{
	int fd;
	int ctl = 0;
	int ret;
	char chip[10];
	int i;

	
	if(argc < 2){
		print_usage(argv[0]);
		return -1;
	}
	
	num = atoi(argv[1]);
	ctl_info.off = atoi(argv[2]);
	if(argc == 4)
	{
		ctl_info.val = atoi(argv[3]);
	} else
	{
		ctl_info.val = 0;
	}

//	printf("ctl_info.off = %d  ctl_info.val = %d\n", ctl_info.off, ctl_info.val);

	if(ctl_info.off < 0 || ctl_info.off > 15)
	{
		printf("\nERROR:pins out of range\nplease input 0-15\n");
		return -1;
	}

	if(num >= 0 && num < 6 || num == 8 || (num == 13 && ctl_info.off == 0) || (num ==14 &&  ctl_info.off == 0) || num ==15 || num == 17 || (num == 19 && ctl_info.off == 0) || (num == 20 && ctl_info.off == 0)) //判断需要打开哪个设备
	{
	//	printf("%s %s %d\n", __FILE__, __func__, __LINE__);
		fd = open("/dev/pca9535_1", O_RDWR);
		if (fd < 0)
		{
			perror("/dev/pca9535_1");
			return -1;
		}

		strcpy(chip, "chip1");

	}else if(num >= 6 && num <= 12 && num != 8 || (num == 13 && ctl_info.off == 1) || (num ==14 &&  ctl_info.off == 1) || num ==16 || num == 18 || (num == 19 && ctl_info.off == 1) || (num == 20 && ctl_info.off == 1))
	{
	//	printf("%s %s %d\n", __FILE__, __func__, __LINE__);
		fd = open("/dev/pca9535_2", O_RDWR);
		if (fd < 0)
		{
			perror("/dev/pca9535_2");
			return -1;
		}

		strcpy(chip, "chip2");
	}else
	{
		printf("invalid number\n");
		print_usage(argv[0]);
		return -1;
	}
	printf("++++++++  %d  +++++++++\n", num);
	if(num == 5 ||num == 12 || num >= 14 && num <= 16 || num == 19 )
	{
	//	printf("%s %d  argv[3]=%s\n", __func__, __LINE__,  argv[3]);

		hex2binary(argv[3]);
	}
	
	ret = ioctl(fd, num, &ctl_info);
	
	if(ret < 0)
	{
		perror("ioctl");
		return -1;
	}
	if(num == 0 || num == 6)
	{
		printf("Received 1 byte data for %s\n", chip);
		printf("%s valus is %d\n", chip, ctl_info.val );
	}

	if(num == 4 || num == 11)
	{
		printf("Received 8 byte data for %s\n", chip);
		for(i = 0; i < 8; i++)
		{
			printf("%d  ", ctl_info.byte_data[i]);
		}
		printf("\n");
	}
	else if(num == 13)
	{
		printf("Received 16 byte data for %s\n", chip);
		for(i = 0; i < 16; i++)
		{
			printf("%d ",ctl_info.d_byte_data[i]);
		}
		printf("\n");
	}

	close(fd);
	return 0;
}
