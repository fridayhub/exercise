#include "include.h"
#include "service.h"

//根据Tanda.conf的配置信息修改
/****************************************************/
extern char softVersion[50];
extern char hardVersion[50];
extern char TP_HOST[50];
extern int16_t tp_port; 
extern int controllerID;
extern int controllerType;
extern int16_t uartselect;// 1 is 232 , 2 is 485 , 3 is all
extern int  baudrate232;
extern int  baudrate485;
extern char dynamicName232[50];
extern char dynamicName485[50];
extern char systype[10];
/****************************************************/

// 配置结构体
struct tandaConfig 
{
    char key[20];
    char value[50];
};

//save config options
struct tandaConfig configList[]=  
{
    {{"HardVersion"},{0}},
    {{"SoftVersion"},{0}},
	{{"ControllerType"},{0}},
	{{"ControllerID"},{0}},
    {{"IMEI"},{0}},
    {{"ip"}, {0}},
    {{"port"},{0}},
    {{"portenable"}, {0}},
    {{"baudrate232"}, {0}},
    {{"protocolName232"}, {0}},
    {{"baudrate485"},{0}},
	{{"protocolName485"},{0}},
};


/*
 * 字符串中寻找以等号分开的键值对
 * @param  src  源字符串 [输入参数]
 * @param  key     键    [输出参数]
 * @param value    值    [输出参数]
 */
static int strkv(char *src, char *key, char *value)
{
    char *p,*q;
 
    if(*src == '#') return 0; // # 号开头为注视，直接忽略
 
    p = strchr(src, '=');   // p找到等号
    q = strchr(src, '\n');  // q找到换行
 
    // 如果有等号有换行
    if (p != NULL && q != NULL)
    {
        *q = '\0'; // 将换行设置为字符串结尾
        strncpy(key, src, p - src); // 将等号前的内容拷贝到 key 中
        strcpy(value, p+1); // 将等号后的内容拷贝到 value 中
        return 1;
    }else
    {
        return 0;
    }
}
 
//int main()
int readConfigOptions(char *FileName)
{
	int i;
	FILE *fd;
	char buf[50]="";	// 缓冲字符串
	char key[50]="";	// 配置变量名
	char value[50]="";	// 配置变量值	 
 	int configNum=sizeof(configList)/sizeof(struct tandaConfig);
	// 打开配置文件
	fd = fopen(FileName, "r");
 
	if (fd == NULL)
	{
		printf("Can't open config file\n");
		return 0;

	}
 
	// 依次读取文件的每一行
	while(fgets(buf, 50, fd))
	{
		// 读取键值对
		if (strkv(buf, key, value))
		{
			// 读取成功则循环与配置数组比较
			for(i = 0; i< configNum; i++)
			{
				// 名称相等则拷贝
				if (strcmp(key, configList[i].key) == 0)
				{				
					strcpy(configList[i].value, value);
				}				
			}
			// 清空 读取出来的 key
			memset(key, 0, strlen(key));
		}
	}
 
	fclose(fd);
	return 1;
}


void analysisOptions(void)
{
    int i;
	
 	int configNum=sizeof(configList)/sizeof(struct tandaConfig);
    for(i = 0; i < configNum; i++)
    {
        printf("%s = %s \n", configList[i].key, configList[i].value);
    }

    for(i = 0; i < configNum; i++)
	{
       if(strcmp(configList[i].key, "HardVersion") == 0)
 	    	strcpy(hardVersion,  configList[i].value);
       else if(strcmp(configList[i].key, "SoftVersion") == 0)
	    	strcpy(softVersion,  configList[i].value);
	   else if(strcmp(configList[i].key, "ControllerType") == 0)
	   		controllerType = atoi(configList[i].value);
	   else if(strcmp(configList[i].key, "ControllerID") == 0)
	   		controllerID = atoi(configList[i].value);
       else if(strcmp(configList[i].key, "IMEI") == 0)
	   		printf("####%s\n",configList[i].value);
       else if(strcmp(configList[i].key, "ip") == 0)
           	strcpy(TP_HOST,  configList[i].value);
       else if(strcmp(configList[i].key, "port") == 0)
           	tp_port = atoi(configList[i].value);
       else if(strcmp(configList[i].key, "portenable") == 0)
           	uartselect = atoi(configList[i].value);
       else if(strcmp(configList[i].key, "baudrate232") == 0)
       		baudrate232 = atoi(configList[i].value);
       else if(strcmp(configList[i].key, "baudrate485") == 0)
           	baudrate485= atoi(configList[i].value);
       else if(strcmp(configList[i].key, "protocolName232") == 0)
	   	   	strcpy(dynamicName232,configList[i].value);
       else if(strcmp(configList[i].key, "protocolName485") == 0)
		   	strcpy(dynamicName485,configList[i].value);
	}
	
	if(uartselect ==2)
	{
		properties.protocolType = 1; //1 485方式
		printf("Use RS485 !\n");
	}
	else if(uartselect ==1)
	{
		properties.protocolType = 2; //2 232方式
		printf("Use RS232 !\n");
	}
	else
	{
		printf("Use ALL !\n");
	}
}

/* read mac id from *eth  */
int getThingMac(char *mac)
{
    char *eth = "eth0"; //name need read of network card
    struct ifreq ifreq;
    int sock = 0;
    int i = 0;

    sock = socket(AF_INET, SOCK_STREAM,0);
    if(sock < 0)
    {
        perror("error sock");
        return 2;
    }

    strcpy(ifreq.ifr_name, eth);
    if(ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0)
    {
        perror("error ioctl");
        return 3;
    }

    for(i = 0; i < 6; i++){
        sprintf(mac+2*i, "%02X:", (unsigned char)ifreq.ifr_hwaddr.sa_data[i]);
    }
    mac[strlen(mac) - 1] = 0;
    close(sock);

    return 0;
}

#if 0
int readAdaptorID(char *adaptorID)
{
    int fdr;
    char buf[50] = "";    // 缓冲字符串
    char mac[20] = ""; 

    memset(buf, 0, strlen(buf));

    fdr = open("/data/bin/mac", O_RDWR|O_CREAT);
    if (fdr < 0)
    {
        printf("Can't open config file to read mac\n");
        return 0;
    }

    // 读取一行
    read(fdr, buf, 50);

    if(strlen(buf) > 0){  //配置文件中有存mac地址
        strcpy(mac, buf);
    }else{
        getThingMac(mac);
        write(fdr, mac, strlen(mac));
    }
    // 清空 读取出来的 key
    //printf("read mac is %s\n", mac);
    strcpy(adaptorID, "01_TX3251_");
    strcat(adaptorID, mac);
    return 1;
}
#endif


int readAdaptorID(char *adaptorID, char *systype)
{
    char buf1[10], buf2[10];
    char bufUid[20];
    int fd = 0, i = 0, j = 0;

    fd = open("/sys/fsl_otp/HW_OCOTP_CFG0", O_RDONLY);
    if(fd < 0){
        perror("Open /sys/fsl_otp/HW_OCOTP_CFG0");
        return 0;
    }

    read(fd, buf1, 10);
    for(i = 0, j = 2; j < 10; i++, j++){
        if(buf2[j] != '\n'){
            bufUid[i] = buf1[j];
        }else
            break;
    }
    //printf("read id  1 is :%s\n", buf1);
    close(fd);


    fd = open("/sys/fsl_otp/HW_OCOTP_CFG1", O_RDONLY);
    if(fd < 0){
        perror("Open /sys/fsl_otp/HW_OCOTP_CFG1");
        return 0;
    }

    read(fd, buf2, 10);
    for(j = 2; j < 10; i++, j++){
        if(buf2[j] != '\n'){
            bufUid[i] = buf2[j];
        }else
            break;
    }
    bufUid[i] = '\0';
    //printf("read id  2 is :%s\n", buf2);
    printf("read cpuID   is :%s\n", bufUid);
    close(fd);

    strcpy(adaptorID, "01_TX3251_");
    strcat(adaptorID, bufUid);
    return 1;
}
