#include "protocol.h"
#include "../../../service.h"

extern volatile int ledFlag;

static unsigned char RxBuffer[RxBufferSize]={0};
static unsigned char TxBuffer[TxBufferSize]={0};
static unsigned int TxCounter=0;
static unsigned short Swift_Number=0;
static unsigned short Swift_Number_bk=0;//当流水号一样就不存储数据
//远程控制
static unsigned char RemoteTxBuffer[TxBufferSize]={0};
static unsigned int Remote_TxCounter=0;
static unsigned int Remote_RetryTicks=REMOTE_RETRY_TIMES;//远程控制重发计数
twData tw_TANDA3016_Info;
static int MAXLEN = 990 + 30;
application_info head=NULL;
char *Catch_Path = "./update/Catch_Tanda3016_485.bin";

void Tanda_SendOlder(unsigned char *RxBuffer, int flag , int fd)
{
	
	int i;
	unsigned char checksum=0;	
	memset(TxBuffer , 0 , TxBufferSize);
	tcflush(fd, TCOFLUSH); //清空发送缓存
	MemCpy(TxBuffer , RxBuffer , 24);	
	TxBuffer[2] = RxBuffer[2];//业务流水号低
	TxBuffer[3] = RxBuffer[3];//业务流水号高
	TxBuffer[4] = MAIN_VERSION;
	TxBuffer[5] = SUB_VERSION;
	TxBuffer[24] = 0x00;
	TxBuffer[25] = 0x00;
	TxBuffer[26] = flag;
	for(i=2 ; i<27 ; i++)
	{
		checksum += TxBuffer[i];
	}
	TxBuffer[27]=checksum;
	TxBuffer[28]='#';
	TxBuffer[29]='#';
	TxCounter=30;
	send_data(fd, TxBuffer, TxCounter);
}

int TandaCheckDat(int revlen)
{
	unsigned char checksum=0;
	int i , j;
	printf("revdat485:\n");
	for(j=0 ; j<revlen ; j++)
	{
		printf("%x " , RxBuffer[j]);
	}
	printf("\n");
	if(revlen==0)
	{
		return -1;
	}
	if(RxBuffer[0]!='@'&&RxBuffer[1]!='@')//err should reload
	{
		printf("\n head  err\n");
		return -2;
	}
    revlen-=3;
    for(i=2 ; i<revlen ; i++)
    {
        checksum+=RxBuffer[i];
    }
    if(checksum!=RxBuffer[revlen])		//checksum err should reload
    {
        printf("\nchekcsum err %d %x %x\n" ,revlen+3 ,checksum , RxBuffer[revlen]);
        return -2;
    }
    
}

int Tanda_ReadDat(int fd1)
{
	memset(RxBuffer, 0, sizeof(RxBuffer));
	int ret=0, nread;
	fd_set fds;
	int count = 0;
	struct timeval tv;
    unsigned short len = 0; //calculate
	static unsigned char readmsg;
	int read_enable=0;
	while (1)
	{
		FD_ZERO(&fds);
		FD_SET(fd1, &fds);

		//timeout setting
		tv.tv_sec =  2;
		tv.tv_usec = 0;
		ret = select(fd1 + 1, &fds, NULL, NULL, &tv);
		if(ret < 0)
		{
			ret=0;
			break;
		}
		else if(ret == 0)
		{
			ret= 0;
			break;
		}

		if(FD_ISSET(fd1, &fds))
		{			
			readmsg=0;
			nread = read(fd1, &readmsg, 1);
			if(nread > 0)
			{
                ledFlag = 1; //led 闪烁
				if('@' == readmsg)
				{
					read_enable=1;
				}
				if(read_enable)
				{
                    RxBuffer[count++] = readmsg;
                    if(count > 25)
                    {
                        len = (RxBuffer[24] + (RxBuffer[25]<<8)) + 30;
                        if(count >= len || count >= MAXLEN)
                            return count;
                        //if(RxBuffer[count-2] == '#' && RxBuffer[count-1] == '#')
                        //{
                        //    return count;
                        //}
                    }
				}
			}
		}
	}
	return ret;
}
int Tanda_ProcessDat(void)
{
	int i,j;	
	int infonum=RxBuffer[28];
    if(Swift_Number_bk==Swift_Number)//流水号和上一包一样，则不存储这一数据
    {
        return;
    }
	switch(RxBuffer[27])
	{
		case PARTS_RUNNING_STATE:
		{			
			if(infonum>19)
			{
				return FALSE;
			}
			for(i=0 ; i<infonum ; i++)	
			{		
				tw_TANDA3016_Info.deviceType=RxBuffer[29+61*i];			//55+6  55bytes info with 6 bytes timetick
				tw_TANDA3016_Info.deviceAddr[0]=RxBuffer[31+61*i];
				tw_TANDA3016_Info.deviceAddr[1]=RxBuffer[30+61*i];
				tw_TANDA3016_Info.componType=RxBuffer[32+61*i];
				tw_TANDA3016_Info.componLoopID[0]=RxBuffer[34+61*i];
				tw_TANDA3016_Info.componLoopID[1]=RxBuffer[33+61*i];
				if((0 == tw_TANDA3016_Info.componLoopID[0] && 0 == tw_TANDA3016_Info.componLoopID[1]) || tw_TANDA3016_Info.componLoopID[1] == 0xFF)
					tw_TANDA3016_Info.servicetype=SET_CONTROLLER_STATE;
				else
					tw_TANDA3016_Info.servicetype=SET_DEVICE_STATE;
				tw_TANDA3016_Info.componAddrs[0]=RxBuffer[36+61*i];
				tw_TANDA3016_Info.componAddrs[1]=RxBuffer[35+61*i];
				tw_TANDA3016_Info.channelNum = RxBuffer[37+61*i];
				tw_TANDA3016_Info.componStatus[0]=RxBuffer[43+61*i];
				tw_TANDA3016_Info.componStatus[1]=RxBuffer[42+61*i];
				for(j=0 ; j<6 ; j++)
				{
					tw_TANDA3016_Info.occuTime[j] = RxBuffer[89+i*61-j];
				}
                if(tw_TANDA3016_Info.componStatus[1] == 1)
                    insert_rxbuffer_data_head(head, tw_TANDA3016_Info);
                else
                    insert_rxbuffer_data(head, tw_TANDA3016_Info);
			}
			break;
		}
		case DEVICE_OPER_INFO:
		{
			if(infonum>98)
			{
				return FALSE;
			}
			for(i=0 ; i<infonum ; i++)
			{
				tw_TANDA3016_Info.deviceType = RxBuffer[29+10*i];
				tw_TANDA3016_Info.deviceAddr[0] = RxBuffer[31+10*i];
				tw_TANDA3016_Info.deviceAddr[1] = RxBuffer[30+10*i];
				tw_TANDA3016_Info.deviceOperType = RxBuffer[32+10*i];
				for(j=0 ; j<6 ; j++)
				{
					tw_TANDA3016_Info.occuTime[j]=RxBuffer[38+10*i-j];
				}
				tw_TANDA3016_Info.servicetype=SET_OPERATOR_INFO;
				insert_rxbuffer_data(head,tw_TANDA3016_Info);
			}
			break;
		}
		case READ_REMOTE_LEVEL_STATUS:
		{			
			tw_TANDA3016_Info.deviceType = RxBuffer[29];
			tw_TANDA3016_Info.deviceAddr[0] = RxBuffer[31];
			tw_TANDA3016_Info.deviceAddr[1] = RxBuffer[30];
			tw_TANDA3016_Info.AuthorizationStatus = RxBuffer[32];
			for(j=0 ; j<12 ; j++)
			{
				tw_TANDA3016_Info.AuthorizationId[j]=RxBuffer[33+j];
			}
			tw_TANDA3016_Info.servicetype=READ_REMOTE_LEVEL_STATUS;
			insert_rxbuffer_data(head,tw_TANDA3016_Info);
			break;
		}
		case READ_CONTROLLER_ID:
		{
			tw_TANDA3016_Info.deviceType = RxBuffer[29];
			tw_TANDA3016_Info.deviceAddr[0] = RxBuffer[31];
			tw_TANDA3016_Info.deviceAddr[1] = RxBuffer[30];
			for(j=0 ; j<12 ; j++)
			{
				tw_TANDA3016_Info.AuthorizationId[j]=RxBuffer[32+j];
			}
			tw_TANDA3016_Info.servicetype=READ_CONTROLLER_ID;	
			insert_rxbuffer_data(head,tw_TANDA3016_Info);
			break;
		}
		
		case READ_REMOTE_STATUS:
		{
			tw_TANDA3016_Info.deviceType = RxBuffer[29];
			tw_TANDA3016_Info.deviceAddr[0] = RxBuffer[31];
			tw_TANDA3016_Info.deviceAddr[1] = RxBuffer[30];
			tw_TANDA3016_Info.AuthorizationControlType = RxBuffer[32];
			tw_TANDA3016_Info.indicate = RxBuffer[33];
			tw_TANDA3016_Info.servicetype=READ_REMOTE_STATUS;			
			insert_rxbuffer_data(head,tw_TANDA3016_Info);
			break;
		}
		default:
			break;
	}
	return TRUE;
}
/*****************************************************************************/
/******************             485部分					****************/
/*****************************************************************************/


int Tanda_Get485_Data(int fd)
{
	int ret , i;
	int revlen=0;
	
	revlen=Tanda_ReadDat(fd);
	ret=TandaCheckDat(revlen);	
    ledFlag = 0; //led 停止闪烁
	if(ret==-1)			//should send polling older
	{	
		
		return -1; //no date get
	}
	if(ret==-2)			//send reload older
	{	
		//if connect is 232  we should repact NAK to controller,if is 485,we just keep silent and wait controller send again
		return -1; //no date get
	}
	
	usleep(800000);		// wait controller change rx mode when we send the data
	ret=Set_485TX_Enable(fd);
	if(ret==-1)
	{
		perror("err config 485 to read");
	}
	Swift_Number=RxBuffer[2]+(RxBuffer[3]<<8);
	if(RxBuffer[26]==CONTROL_OLDER_DATA)		//发送数据
	{		
		if(RxBuffer[27]==TEST)//如果存在远程控制数据待发送,则在控制器发送09巡检后不在恢复确认,恢复远程控制即可
		{
			if(Remote_TxCounter)
			{
				TxCounter=Remote_TxCounter;
				MemCpy(TxBuffer , RemoteTxBuffer, Remote_TxCounter);	
				tcflush(fd, TCOFLUSH); //清空发送缓存
				send_data(fd , TxBuffer , TxCounter);	
				Remote_RetryTicks--;
				if(!Remote_RetryTicks)
				{
					Remote_TxCounter=0;
					Remote_RetryTicks=REMOTE_RETRY_TIMES;
				}
			}
			else
			{
				Tanda_SendOlder(RxBuffer,CONTROL_OLDER_ACK,fd);
			}
		}
		else
		{
			ret=Tanda_ProcessDat();
			Tanda_SendOlder(RxBuffer,CONTROL_OLDER_ACK,fd);
		}
	}
	else if (RxBuffer[26]==CONTROL_OLDER_REPLY)
	{		
		Remote_TxCounter=0; 
		Remote_RetryTicks=REMOTE_RETRY_TIMES;
		ret=Tanda_ProcessDat();
		Tanda_SendOlder(RxBuffer,CONTROL_OLDER_ACK,fd);
	}
	else if (RxBuffer[26]==CONTROL_OLDER_ACK)	//确认
	{
	}
	else if (RxBuffer[26]==CONTROL_OLDER_NAK)	//返回NAK 重传上一帧
	{		
		tcflush(fd, TCOFLUSH); //清空发送缓存
		send_data(fd , TxBuffer , TxCounter);	
	}
    Swift_Number_bk=Swift_Number;	
	
	usleep(800000);		// wait controller change rx mode when we send the data
	ret=Set_485RX_Enable(fd);
	if(ret==-1)
	{
		perror("err config 485 to read");
	}
	
	memset(RxBuffer , 0 , RxBufferSize);
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
	delete_rxbuffer_data(head); //删除节点
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
	int i=0 , recvlen=0;
	int ret=0;
	if(fd<0)
	{
		return 1;
	}
    printf("\n_in 485_\n");
	if(PORT_RS485==port)
		ret=Tanda_Get485_Data(fd);
	else
		;//ret=Tanda_Get485_Data();

    printf("\n_out 485_\n");
}

/*远程控制关机 黑屏等*/
void tw_RemoteControl(int controllerid, int controllertype, int controltype, int controlmarker)//远程控制
{
	int x, i;
	unsigned char checksum=0;
	if(Remote_TxCounter && Remote_RetryTicks!=REMOTE_RETRY_TIMES)
	{
		return;
	}
	memset(RemoteTxBuffer, 0, sizeof(TxBufferSize));
	RemoteTxBuffer[0] = '@';
	RemoteTxBuffer[1] = '@';
	RemoteTxBuffer[2] = 0;//(serialNumber)&0xff; //业务流水号低
	RemoteTxBuffer[3] = 1;//(serialNumber)>>8; //业务流水号高
	RemoteTxBuffer[4] = MAIN_VERSION;
	RemoteTxBuffer[5] = SUB_VERSION;
	
	RemoteTxBuffer[24] = 8;  //应用数据单元长度低字节
	RemoteTxBuffer[25] = 0;  //应用数据单元长度高字节
	RemoteTxBuffer[26] =CONTROL_OLDER_REQUIRE;//2发送火灾自动报警系统火灾报警、运行状态等信息
	RemoteTxBuffer[27] = REMOTE_CONTROLLER;  //远程授权  
	RemoteTxBuffer[28] = 1; //信息对象数目
	RemoteTxBuffer[29] = controllertype;  //控制器类型
	RemoteTxBuffer[30] = controllerid & 0xff; //设备地址
	RemoteTxBuffer[31] = controllerid >> 8; //设备地址高  
	RemoteTxBuffer[32] = controltype; //控制类型
	RemoteTxBuffer[33] = controlmarker; //控制指示
	Remote_TxCounter = 34;
	for(i = 2; i < Remote_TxCounter; i++)
	{
		 checksum+= RemoteTxBuffer[i];  //累加和
	}
	RemoteTxBuffer[Remote_TxCounter]=checksum;
	Remote_TxCounter++;
	RemoteTxBuffer[Remote_TxCounter++] = '#';
	RemoteTxBuffer[Remote_TxCounter++] = '#';
	
}

/*远程控制禁止和使能*/
void tw_setRemoteControlStatus(int controllerid, int controllertype, int oparatorstatus)//远程控制
{
	int x, i;
	unsigned char checksum=0;
	if(Remote_TxCounter && Remote_RetryTicks!=REMOTE_RETRY_TIMES)
	{
		return;
	}
	memset(RemoteTxBuffer, 0, sizeof(TxBufferSize));
	RemoteTxBuffer[0] = '@';
	RemoteTxBuffer[1] = '@';
	RemoteTxBuffer[2] = 0;//(serialNumber)&0xff; //业务流水号低
	RemoteTxBuffer[3] = 1;//(serialNumber)>>8; //业务流水号高
	RemoteTxBuffer[4] = MAIN_VERSION;
	RemoteTxBuffer[5] = SUB_VERSION;
	
	RemoteTxBuffer[24] = 7;  //应用数据单元长度低字节
	RemoteTxBuffer[25] = 0;  //应用数据单元长度高字节
	RemoteTxBuffer[26] =CONTROL_OLDER_REQUIRE;//2发送火灾自动报警系统火灾报警、运行状态等信息
	RemoteTxBuffer[27] = SET_REMOTE_ENABLE_STATUS;  //远程授权  
	RemoteTxBuffer[28] = 1; //信息对象数目
	RemoteTxBuffer[29] = controllertype;  //控制器类型
	RemoteTxBuffer[30] = controllerid & 0xff; //设备地址
	RemoteTxBuffer[31] = controllerid >> 8; //设备地址高  
	RemoteTxBuffer[32] = oparatorstatus; 	//禁止或开启
	Remote_TxCounter = 33;
	for(i = 2; i < Remote_TxCounter; i++)
	{
		 checksum+= RemoteTxBuffer[i];  //累加和
	}
	RemoteTxBuffer[Remote_TxCounter]=checksum;
	Remote_TxCounter++;
	RemoteTxBuffer[Remote_TxCounter++] = '#';
	RemoteTxBuffer[Remote_TxCounter++] = '#';
	
}

void tw_readRemoteController(int controllerid, int controllertype, int level, char *code, int readType)
{
	int x, i;	
	unsigned char checksum=0;
	if(Remote_TxCounter && Remote_RetryTicks!=REMOTE_RETRY_TIMES)
	{
		return;
	}
	memset(RemoteTxBuffer, 0, sizeof(TxBufferSize));
	Remote_TxCounter=0;
	RemoteTxBuffer[0] = '@';
	RemoteTxBuffer[1] = '@';
	RemoteTxBuffer[2] = 0;//(serialNumber)&0xff; //业务流水号低
	RemoteTxBuffer[3] = 1;//(serialNumber)>>8; //业务流水号高
	RemoteTxBuffer[4] = MAIN_VERSION;
	RemoteTxBuffer[5] = SUB_VERSION;
	RemoteTxBuffer[26] = CONTROL_OLDER_REQUIRE;  //2发送火灾自动报警系统火灾报警、运行状态等信息
	RemoteTxBuffer[27] = readType;	//读取类型
	RemoteTxBuffer[28] = 1; //信息对象数目
	RemoteTxBuffer[29] = controllertype; //设备类型
	RemoteTxBuffer[30] = controllerid & 0xff; //设备地址
	RemoteTxBuffer[31] = controllerid >> 8; //设备地址高  
	Remote_TxCounter = 32;
	x = Remote_TxCounter - 27;
	RemoteTxBuffer[24] = x & 0xff; //应用数据单元长度低字节
	RemoteTxBuffer[25] = x >> 8; //应用数据单元长度高字节
	for(i = 2; i < Remote_TxCounter; i++)
	{
		checksum += RemoteTxBuffer[i];  //累加和
	}
	RemoteTxBuffer[Remote_TxCounter]=checksum;
	Remote_TxCounter++;
	RemoteTxBuffer[Remote_TxCounter++] = '#';
	RemoteTxBuffer[Remote_TxCounter++] = '#';
}




void tw_setRemoteController(int controllerid, int controllertype, int level, char *code)
{
	int x, i;
	unsigned char checksum=0;
	if(Remote_TxCounter && Remote_RetryTicks!=REMOTE_RETRY_TIMES)
	{
		return;
	}
	memset(RemoteTxBuffer, 0, sizeof(TxBufferSize));
	Remote_TxCounter=0;
	RemoteTxBuffer[0] = '@';
	RemoteTxBuffer[1] = '@';
	RemoteTxBuffer[2] = 0;//(serialNumber)&0xff; //业务流水号低
	RemoteTxBuffer[3] = 1;//(serialNumber)>>8; //业务流水号高
	RemoteTxBuffer[4] = MAIN_VERSION;
	RemoteTxBuffer[5] = SUB_VERSION;
	RemoteTxBuffer[26] =CONTROL_OLDER_REQUIRE;//2发送火灾自动报警系统火灾报警、运行状态等信息
	RemoteTxBuffer[27] = REMOTE_ENABLE;  //远程授权  
	RemoteTxBuffer[28] = 1; //信息对象数目
	RemoteTxBuffer[29] = 1; //设备类型 远程授权
	RemoteTxBuffer[30] = controllerid & 0xff; //设备地址
	RemoteTxBuffer[31] = controllerid >> 8; //设备地址高  
	RemoteTxBuffer[32] = level; //授权级别
	//授权码
	for(i=0; i<32; i++)
	{
		RemoteTxBuffer[33+i] = code[i];
	}
	//33 - 64 授权码
	Remote_TxCounter = 65;
	x = Remote_TxCounter - 27;
	RemoteTxBuffer[24] = x & 0xff; //应用数据单元长度低字节
	RemoteTxBuffer[25] = x >> 8; //应用数据单元长度高字节
	for(i = 2; i < Remote_TxCounter; i++)
	{
		checksum+=RemoteTxBuffer[i];  //累加和
		
	}
	RemoteTxBuffer[Remote_TxCounter] =checksum;
	Remote_TxCounter++;
	RemoteTxBuffer[Remote_TxCounter++] = '#';
	RemoteTxBuffer[Remote_TxCounter++] = '#';	
}

twInfoTable* CreateReadRemoteStatusInfoTable(application_info app)
{
	/* Make the request to the server */
	twInfoTable * content = NULL;
	twInfoTableRow *row = 0;
	twDataShape * ds = 0;
	unsigned char controllerID[8] = {0}, controllerType[5] = {0},ControlType[8] ={0};
	ds = twDataShape_Create(twDataShapeEntry_Create("ControllerID", NULL, TW_STRING));
	if (!ds) 
	{
		TW_LOG(TW_ERROR, "Error Creating datashape.");
		return NULL;
	}
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerType", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControlType", NULL, TW_STRING));
	content = twInfoTable_Create(ds);
	if (!content) {
		TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable");
		twDataShape_Delete(ds); 
		return NULL;
	}
	itoan(app->data.deviceAddr, controllerID, 2); //控制器ID
	
	itoa(app->data.deviceType, controllerType);  //控制器类型
	itoa(app->data.AuthorizationControlType, ControlType); 	//控制类型
	row = twInfoTableRow_Create(twPrimitive_CreateFromString(controllerID, TRUE));
	if (!row) 
	{
		TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable row");
		twInfoTable_Delete(content);
		return NULL;
	}
	/* populate the InfoTableRow with arbitrary data */
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(controllerType, TRUE));			 //ControllerType
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(ControlType, TRUE));			 //controller  Authorization Status System ID
	/* add the InfoTableRow to the InfoTable */
	twInfoTable_AddRow(content, row);
	return content;
}


twInfoTable* CreateReadRemoteUDIDInfoTable(application_info app)
{
	/* Make the request to the server */
	twInfoTable * content = NULL;
	twInfoTableRow *row = 0;
	twDataShape * ds = 0;
	unsigned char controllerID[8] = {0}, controllerType[10] = {0},controllerUuid[20] ={0};
	ds = twDataShape_Create(twDataShapeEntry_Create("ControllerID", NULL, TW_STRING));
	if (!ds) {
		TW_LOG(TW_ERROR, "Error Creating datashape.");
		return NULL;
	}
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerType", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("UDID", NULL, TW_STRING));
	content = twInfoTable_Create(ds);
	if (!content) {
		TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable");
		twDataShape_Delete(ds); 
		return NULL;
	}
	itoan(app->data.deviceAddr, controllerID, 2); //控制器ID
	itoan(app->data.AuthorizationId, controllerUuid,12); //授权控制器系统ID
	itoa(app->data.deviceType, controllerType);  //控制器类型
	row = twInfoTableRow_Create(twPrimitive_CreateFromString(controllerID, TRUE));
	if (!row) {
		TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable row");
		twInfoTable_Delete(content);
		return NULL;
	}
	/* populate the InfoTableRow with arbitrary data */
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(controllerType, TRUE));			 //ControllerType
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(controllerUuid, TRUE));			//controller  Authorization Status System ID
	/* add the InfoTableRow to the InfoTable */
	twInfoTable_AddRow(content, row);

	return content;
}

twInfoTable* CreateReadRemoteLEVELInfoTable(application_info app)
{
	/* Make the request to the server */

	twInfoTable * content = NULL;
	twInfoTableRow *row = 0;
	twDataShape * ds = 0;
	unsigned char controllerID[8] = {0}, controllerType[10] = {0},controllerUuid[20] ={0};
	int level1Status=2 ,level2Status=2;
	ds = twDataShape_Create(twDataShapeEntry_Create("ControllerID", NULL, TW_STRING));
	if (!ds) {
		TW_LOG(TW_ERROR, "Error Creating datashape.");
		return NULL;
	}
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerType", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("Level1Status", NULL, TW_INTEGER));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("Level2Status", NULL, TW_INTEGER));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("UDID", NULL, TW_STRING));
	content = twInfoTable_Create(ds);

	if (!content) {
		TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable");
		twDataShape_Delete(ds); 
		return NULL;
	}	
	itoan(app->data.deviceAddr, controllerID, 2); //控制器ID
	
	itoan(app->data.AuthorizationId, controllerUuid,12); //授权控制器系统ID
	itoa(app->data.deviceType, controllerType);  //控制器类型
	if(app->data.AuthorizationStatus == 1){ //授权等级
		level1Status = 0;
		level2Status = 0;
	}else if(app->data.AuthorizationStatus == 2){
		level1Status = 1;
		level2Status = 0;
	}else if(app->data.AuthorizationStatus == 3){
		level1Status = 0;
		level2Status = 1;
	}else if(app->data.AuthorizationStatus == 4){
		level1Status = 1;
		level2Status = 1;
	}
	row = twInfoTableRow_Create(twPrimitive_CreateFromString(controllerID, TRUE));
	if (!row) {
		TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable row");
		twInfoTable_Delete(content);
		return NULL;
	}
	/* populate the InfoTableRow with arbitrary data */
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(controllerType, TRUE));			 //ControllerType
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromInteger(level1Status));			  //level1Status
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromInteger(level2Status));			  //level2Status
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(controllerUuid, TRUE));			//controller  Authorization Status System ID
	/* add the InfoTableRow to the InfoTable */
	twInfoTable_AddRow(content, row);
	return content;
}

twInfoTable* tw_CreateUARTInfoTable(DATETIME now,int Num , tw_Public *ptr)
{
	/* Make the request to the server */
	
	twInfoTable * content = NULL;
	twInfoTableRow *row = 0;
	twDataShape * ds = 0;
	char now_time[25];

    long long linuxTime = 0;
    char currentTime[25];

    unsigned char controllerID[8] = {0}, controllerType[5] = {0},deviceType[5] = {0}, deviceStatus[8] = {0};
	unsigned char operatorMark[5] = {0}, deviceAddr[30] = {0}, deviceLoopID[8] = {0};
	unsigned char channelID[5] = {0}, controllerUuid[50] = {0};
	application_info app;
	app=get_rxbuffer_data(head);
	if(NULL == app)
	{
		get_catch_file(head);
		app=get_rxbuffer_data(head);
		if(NULL == app)
			return NULL;
	}
	ptr->ServerType=app->data.servicetype;
	if(READ_REMOTE_LEVEL_STATUS == ptr->ServerType)	//读远程授权等级
	{
		content = CreateReadRemoteLEVELInfoTable(app);
		return content;
	}
	if(READ_CONTROLLER_ID == ptr->ServerType)				//识别码
	{
		content = CreateReadRemoteUDIDInfoTable(app);
		return content;
	}
	if(READ_REMOTE_STATUS == ptr->ServerType)			//授权状态
	{
		content = CreateReadRemoteStatusInfoTable(app);
		return content;
	}
	ds = twDataShape_Create(twDataShapeEntry_Create("ControllerID", NULL, TW_STRING));
	if (!ds) 
	{
		TW_LOG(TW_ERROR, "Error Creating datashape.");
		return NULL;
	}
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("Flag", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerType", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceLoopID", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceType", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceAddr", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("ChannelID", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceState", NULL, TW_STRING));

    if(app->data.servicetype == SET_OPERATOR_INFO)
	{
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("OperatorMark", NULL, TW_STRING));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("OperatorPersonID", NULL, TW_STRING));
    }
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("CurrentTime", NULL, TW_STRING));
	
	content = twInfoTable_Create(ds);
	if (!content) 
	{
        TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable");
        twDataShape_Delete(ds); 
        return NULL;
    }
    itoal(now, now_time);	
	linuxTime = getConvertTime(app->data.occuTime);
    linuxTime *= 1000;
    itoal(linuxTime, currentTime);
    TW_LOG(TW_TRACE, "Convert time is :%s", currentTime);
	itoan(app->data.deviceAddr, controllerID, 2);
	itoa(app->data.deviceType, controllerType); //itoa(thingData.deviceType, controllerType);  //控制器类型
	itoan(app->data.componLoopID, deviceLoopID ,2);
	itoa(app->data.componType, deviceType); 	 //itoa(thingData.componType, deviceType);部件类型
	itoan(app->data.componAddrs, deviceAddr, 2); //部件地址
	itoa(app->data.channelNum, channelID); //itoa(thingData.channelNum, channelID); 	 //通道号
	itoan(app->data.componStatus, deviceStatus, 2); //部件状态   	
	itoa(app->data.deviceOperType, operatorMark );
	
	row = twInfoTableRow_Create(twPrimitive_CreateFromString(controllerID, TRUE));
	if (!row) {
	   TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable row");
	   twInfoTable_Delete(content);    
	   return NULL;
	}
	/* populate the InfoTableRow with arbitrary data */

	if(SET_OPERATOR_INFO == app->data.servicetype || SET_CONTROLLER_STATE== app->data.servicetype)
	{
	   twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("1", TRUE));			  // FLAG  2:232, 3:485
	}
	else
	  	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("3", TRUE));			  // FLAG  2:232, 3:485
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(controllerType, TRUE));			//ControllerType
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceLoopID, TRUE));			  //回路号
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceType, TRUE));		   //DeviceType
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceAddr, TRUE));		   //设备地址
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(channelID, TRUE)); 		  //通道号
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceStatus, TRUE));			  //state
	if(SET_OPERATOR_INFO == app->data.servicetype)
	{
		
		TW_LOG(TW_DEBUG, "app->data.deviceOperTypC=%s" , operatorMark);
		twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(operatorMark, TRUE));	  //操作类型
		twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("0", TRUE));	 //操作员编号
	}
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(currentTime, TRUE));		 //time
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
