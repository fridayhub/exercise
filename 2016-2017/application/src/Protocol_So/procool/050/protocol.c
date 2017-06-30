#include "protocol.h"
#include "../../../service.h"
typedef unsigned char u8;
typedef unsigned short u16;
static unsigned char RxBuffer[RxBufferSize]={0};
static unsigned char TxBuffer[TxBufferSize]={0};
static unsigned int TxCounter=0;
static unsigned short Swift_Number=0;
twData tw_TANDA050_Info;
application_info head = NULL;
char *Catch_Path = "./update/Catch_Tanda_050.bin";

extern volatile int ledFlag;

u16 usMBCRC16(unsigned char  *srcMsg, unsigned short dataLen);
//static unsigned char ACK[4] = {0x7E, 0x0, 0x20, 0x7C};
static unsigned char ACK[5] = {0x7E, 0x80, 0x0, 0x70, 0x18};
static unsigned char OK[5] = {0x7E, 0x90, 0x20, 0x7c};

unsigned int typeMap[][2] = {{1, 1}, {2, 83}, {3, 84}, {4, 1}, {5, 60}, {6, 70}, {7, 80}, {8, 82}, {9, 20}, {0x0a, 128}, \
                               {0x0b, 129}, {0x0c, 20}, {0x0d, 25}, {0x0e, 22}, {0x0f, 42}, {0x10, 23}, {0x11, 43}, {0x12, 417}, {0x13, 132}, {0x14, 24}, \
                               {0x15, 107}, {0x16, 21}, {0x17, 0}, {0x18, 83}, {0x19, 135}, {0x1a, 83}, {0x1b, 137}, {0x1c, 28}, {0x1d, 128}, {0x1e, 102}, \
                               {0x1f, 108}, {0x20, 0}, {0x21, 61}, {0x22, 62}, {0x23, 0}, {0x24, 0}, {0x25, 130}, {0x26, 20}, {0x27, 20}, {0x28, 134}, \
                               {0x2a, 62}, {0x2b, 64},{0x2c, 60},{0x2d, 139},{0x2e, 63},{0x2f, 81},{0x31, 108}, \
                               {0x83, 84}, {0x85, 70}, {0x87, 82}, {0x8c, 40}, {0x8d, 45}, {0x92, 335}, {0x93, 133}, {0x94, 44}, {0x95, 235}, {0x96, 41}, \
                               {0x98, 84}, {0x99, 136}, {0x9a, 84}, {0x9b, 138}, {0x9c, 48}, {0x9d, 129}, {0x9e, 230}, {0x9f, 236}, {0xa0, 0}, {0xa5, 131}, \
                               {0xa6, 40}, {0xaa, 72},{0xab, 73},{0xac, 70},{0xad, 0},{0xae, 0}, {0xaf, 82},{0xb1, 236} };

void converType(unsigned int src, unsigned int *dest, twData *tw_TANDA050_Info)
{
    int i = 0;
    //printf("\n\n\n\nsizeof is %d\n\n\n\n\n\n", sizeof(typeMap)/sizeof(typeMap[0]));
    for(i = 0; i < sizeof(typeMap)/sizeof(typeMap[0]); i++){
        if(src == typeMap[i][0]){
            *dest = typeMap[i][1];
            if(src == 4 || src == 10 || src == 11 || (src >= 32 && src <= 36)){
                tw_TANDA050_Info->serverType = 4;  //上传操作信息
            }else{
                tw_TANDA050_Info->serverType = 2; //上传状态信息
            }
            printf("\n\nConvert dest is : %d, type is %d\n", *dest, tw_TANDA050_Info->serverType);
        }
    }
}

static void processRecvData(void)
{
    unsigned int converInfoType = 0; //保存转换后的信息类型
    int i = 0;
    if(RxBuffer[1] == 0x03 && RxBuffer[2] == 0x81){ //主控制器复位
        tw_TANDA050_Info.serverType = 4; //操作信息 
        tw_TANDA050_Info.infoType = 1; //系统复位 
        //tw_TANDA050_Info.dataFlag = 0;
        insert_rxbuffer_data(head,tw_TANDA050_Info);
        printf("Controller reset!\n");
    }else if(RxBuffer[1] == 0x0D && RxBuffer[2] == 0x90){
        tw_TANDA050_Info.deviceAddr = RxBuffer[4];
        tw_TANDA050_Info.extensionNum = RxBuffer[6];
        tw_TANDA050_Info.componLoopID = RxBuffer[3]; //回路号
        converType(RxBuffer[5], &converInfoType, &tw_TANDA050_Info);
        tw_TANDA050_Info.infoType = converInfoType; //设备状态
        for(i = 0; i < 6; i++){
            tw_TANDA050_Info.occurTime[i] = RxBuffer[7 + i];
        }
        //tw_TANDA050_Info.dataFlag = 0;
        if(tw_TANDA050_Info.infoType == 1)
            insert_rxbuffer_data_head(head, tw_TANDA050_Info);
        else
            insert_rxbuffer_data(head, tw_TANDA050_Info);
        printf("analysis recv info data!\n"); 
    }
}

static int Tanda_050dataRecv(int fd1)
{
    printf("\nselect start\n"); 
    int ret, nread;
    fd_set fds;
    int count = 0;
    struct timeval tv;
    static unsigned char readmsg;

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(fd1, &fds);

        //timeout setting
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        ret = select(fd1 + 1, &fds, NULL, NULL, &tv);
        if(ret < 0){
            perror("select");
            break;
        }else if(ret == 0){
            printf("timeout\n");
            return 0;
            //continue;
        }

        if(FD_ISSET(fd1, &fds)){
            readmsg= 0;
            nread = read(fd1, &readmsg, 1);
//            printf("read buf %x\n", readmsg);
            if(nread > 0){
                ledFlag = 1;
                RxBuffer[count++] = readmsg;
                //if(RxBuffer[0] == 0x7E && RxBuffer[count-2] == 0x10 && RxBuffer[count-1] == 0x88){
                if(count > 20){
                    memset(RxBuffer, 0, 1024);
                    return 0; 
                }
                if(RxBuffer[0] == 0x7E && count >= (RxBuffer[1]+2)){
                    return count;
                }
            }
        }
    }
return ret;
}

int Tanda_Get232_Data(int fd)
{
	int i;
	int revlen=0;
unsigned short usCRC16;
	memset(RxBuffer , 0 , RxBufferSize);
	revlen=Tanda_050dataRecv(fd);	
    ledFlag = 0; //led 停止闪烁
    if(revlen > 0){
        for(i = 0; i < revlen; i++){
            printf("%x ", RxBuffer[i]);
        }

        if(RxBuffer[1] == 0x0D && RxBuffer[2] == 0x90){ //公共广播90
            memset(TxBuffer, 0, TxBufferSize);
            TxBuffer[0] = 0x7E;
            TxBuffer[1] = 0x90;
            usCRC16 = usMBCRC16(TxBuffer, 2);
            TxBuffer[2] = (usCRC16 >> 8) & 0xFF;
            TxBuffer[3] = usCRC16 & 0xFF;
            send_data(fd, TxBuffer, 4); //收到数据发送确认
        }else if(RxBuffer[1] == 0x03 && RxBuffer[2] == 0x92){ //查询子机类型 92
            TxBuffer[0] = 0x7E;
            TxBuffer[1] = 0x92;
            TxBuffer[2] = 0x07;
            TxBuffer[3] = 0x0;
            usCRC16 = usMBCRC16(TxBuffer, 4);
            TxBuffer[4] = (usCRC16 >> 8) & 0xFF;
            TxBuffer[5] = usCRC16 & 0xFF;
            send_data(fd, TxBuffer, 6); //收到数据发送确认
        }else if(RxBuffer[1] == 0x03 && RxBuffer[2] == 0x81){ //复位子机 81
            TxBuffer[0] = 0x7E;
            TxBuffer[1] = 0x81;
            usCRC16 = usMBCRC16(TxBuffer, 2);
            TxBuffer[2] = (usCRC16 >> 8) & 0xFF;
            TxBuffer[3] = usCRC16 & 0xFF;
            send_data(fd, TxBuffer, 4); //收到数据发送确认
        }else if(RxBuffer[1] == 0x03 && RxBuffer[2] == 0x80){ //查询子机 80
            TxBuffer[0] = 0x7E;
            TxBuffer[1] = 0x80;
            TxBuffer[2] = 0x0;
            usCRC16 = usMBCRC16(TxBuffer, 3);
            TxBuffer[3] = (usCRC16 >> 8) & 0xFF;
            TxBuffer[4] = usCRC16 & 0xFF;
            send_data(fd, TxBuffer, 5); //收到数据发送确认
        }
        processRecvData();
    }

    return 1;
}

/*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/******************             主任务调用函数					****************/

/*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*********************************************************************************
  *Function:	tw_InitList
  *Description:	初始化存储链表
  *Called By:	主任务
  *Input:	无
  *Output: 无
  *Return: 是否可以分配空间
  *Others: 
  *Author:	lfy
**********************************************************************************/
int tw_InitList(int fd)
{	
	head=init_rxbuffer_data_list();
	if(head==NULL)
	{
		return -1;
	}
	return 0;
}

void tw_DestroyList(void)
{	
    destory_rxbuffer_data_list(head);
	head=NULL;
}

void tw_Delete_CurrentNode(void)
{
    delete_rxbuffer_data(head);
}
/*********************************************************************************
  *Function:	tw_Get232UartInformation
  *Description:	获取控制器上传给Adapter的信息(RS232)
  *Called By:	主任务
  *Input:	文件描述符
  *Output: 无
  *Return: 是否获取成功
  *Others: 
  *Author:	lfy
**********************************************************************************/
int tw_GetUARTInformation(int fd , int port)
{
	int ret=0;
	if(fd<0)
	{
		return 1;
	}
	if(port == PORT_RS232)
		ret = Tanda_Get232_Data(fd);
    return 1;
}

int tw_setRemoteController(int controllerid, int controllertype, int level, char *code)
{
	
}

#if 0
static void itoa (unsigned int n, unsigned char *var)
{
    int i, j, sign;
    int len = 0;
    char s[10];

    if((sign = n) < 0)//记录符号
        n =- n;//使n成为正数
    i = 0;
    do{
        s[i++] = n % 10 + '0';//取下一个数字
    }while ((n /= 10) > 0);//删除该数字

    if(sign < 0)
        s[i++] = '-';
    s[i]= '\0';
    i--;
    for(j = i; j >= 0; j--)//生成的数字是逆序的，所以要逆序输出
    {
        var[len] = s[j];
        //printf("%c", var[k]);
        len++;
    }
}
#endif

twInfoTable* tw_CreateUARTInfoTable(DATETIME now,int Num , tw_Public *ptr)
{
	/* Make the request to the server */
	
	twInfoTable * content = NULL;
	twInfoTableRow *row = 0;
	twDataShape * ds = 0;
	char now_time[25];
	long long linuxTime = 0;
	int ret;
	
    char currentTime[25] = {0};
    unsigned char deviceloopid[10] = {0}, deviceaddr[8] = {0};
    unsigned char controllerID[8] = {0}, deviceStatus[8] = {0}, controllerType[5] = {0}, deviceType[5] = {0},  operatorMark[5] = {0}, operatornum[5] = {0}, deviceAddr[30] = {0};
	application_info app;
	app=get_rxbuffer_data(head);
	if(NULL == app)
	{
		get_catch_file(head);
		app=get_rxbuffer_data(head);
		if(NULL == app)
			return NULL;
	}
	
	ptr->ServerType=app->data.serverType;
	ds = twDataShape_Create(twDataShapeEntry_Create("ControllerID", NULL, TW_STRING));
	if (!ds) 
	{
		TW_LOG(TW_ERROR, "Error Creating datashape.");
		return NULL;
	}

    twDataShape_AddEntry(ds, twDataShapeEntry_Create("Flag", NULL, TW_STRING));
    twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerType", NULL, TW_STRING));
    twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceType", NULL, TW_STRING));
    twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceAddr", NULL, TW_STRING));   //设备地址
    twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceLoopID", NULL, TW_STRING));  //回路地址
    if(app->data.serverType == 2){
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceState", NULL, TW_STRING));
    }else if(app->data.serverType == 4){
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("OperatorMark", NULL, TW_STRING));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("OperatorPersonID", NULL, TW_STRING));
    }
    twDataShape_AddEntry(ds, twDataShapeEntry_Create("CurrentTime", NULL, TW_STRING));

    content = twInfoTable_Create(ds);
    if (!content) {
        TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable");
        twDataShape_Delete(ds); 
        return NULL;
    }

    itoa(app->data.extensionNum, controllerID);  //控制器类型
    itoa(app->data.infoType, deviceStatus); //部件状态
    itoa(app->data.componLoopID, deviceloopid); //回路地址
    itoa(app->data.deviceAddr, deviceaddr);

    row = twInfoTableRow_Create(twPrimitive_CreateFromString(controllerID, TRUE));
    if (!row) {
        TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable row");
        twInfoTable_Delete(content);	
        return NULL;
    }

    if(app->data.serverType == 2){
        linuxTime = getConvertTime(app->data.occurTime);
        linuxTime *= 1000;
        itoal(linuxTime, currentTime);
    }else if(app->data.serverType == 4){

        itoal(now, currentTime);	
        printf("\n\n\nthe board time is %s, now is%llu\n\n\n\n", now_time, now);
    }

    printf("contorllerID: %s, deviceloopid: %s, deviceStatus: %s", controllerID, deviceloopid, deviceStatus);

    /* populate the InfoTableRow with arbitrary data */
    if((app->data.infoType == 1 || app->data.infoType == 2) && app->data.serverType == 4)
    {
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("1", TRUE));             // FLAG  2:232, 3:485
    }else{
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("3", TRUE));             // FLAG  2:232, 3:485
    }
    twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("1", TRUE));                  //控制器类型 1
    twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("0", TRUE));           //DeviceType 0
    twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceaddr, TRUE));           //DeviceType 0
    twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceloopid, TRUE));            //state
    if(app->data.serverType == 2){
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceStatus, TRUE));            //state
    }else if(app->data.serverType == 4){
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceStatus, TRUE));            //operator status
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("0", TRUE));            //persion id default 0
    }
    twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(currentTime, TRUE));        //time
    /* add the InfoTableRow to the InfoTable */
    twInfoTable_AddRow(content, row);
    return content;
}

int tw_SaveList_AsFile(void)
{
	application_info p=NULL;
	FILE *fp = fopen(Catch_Path,"a+");
	if(NULL == fp)
	{
		TW_LOG(TW_TRACE, "*****************Save file Open Falid*****************");
		return FALSE;
	}
	while(1)
	{
		p=get_rxbuffer_data(head);
		if(NULL == p)
		{
			break;
		}
		if(!fwrite(&p->data , sizeof(twData) , 1, fp))
		{
			TW_LOG(TW_TRACE, "*****************Save file Falid*****************");
			break;
		}
		delete_rxbuffer_data(head);
	}
	TW_LOG(TW_TRACE, "*****************destroy****************");
	fclose(fp);
	return TRUE;
}


u16 usMBCRC16(unsigned char  *srcMsg, unsigned short dataLen)
{
	 /* CRC 高位字节值表 */    
    u8 auchCRCHi[256] = {  
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,    
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,    
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,    
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,    
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,    
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,    
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,    
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,    
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,    
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,    
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,    
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,    
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,    
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,    
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,    
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,    
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,    
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,    
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,    
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,    
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,    
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,    
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,    
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,    
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,    
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40  
    };    
      
    u8 auchCRCLo[256] = {  
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,    
    0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,    
    0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,    
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,    
    0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,    
    0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,    
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,    
    0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,    
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,    
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,    
    0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,    
    0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,    
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,    
    0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,    
    0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,    
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,    
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,    
    0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,    
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,    
    0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,    
    0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,    
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,    
    0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,    
    0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,    
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,    
    0x43, 0x83, 0x41, 0x81, 0x80, 0x40  
    };  
      
    u8 uchCRCHi = 0xFF ; /* 高CRC字节初始化 */    
    u8 uchCRCLo = 0xFF ; /* 低CRC 字节初始化 */    
    unsigned uIndex = 0; /* CRC循环中的索引 */    
      
    while (dataLen--) /* 传输消息缓冲区 */    
    {    
        uIndex = uchCRCHi ^ *srcMsg++ ; /* 计算CRC */    
        uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex] ;    
        uchCRCLo = auchCRCLo[uIndex] ;    
    }    
    return (u16)((u16)uchCRCHi << 8 | uchCRCLo) ;    
}    


