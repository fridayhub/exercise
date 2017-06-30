#include "protocol.h"
#include "../../../service.h"
static unsigned char RxBuffer[RxBufferSize]={0};
static unsigned char TxBuffer[TxBufferSize]={0};
static unsigned int TxCounter=0;
static unsigned short Swift_Number=0;
static unsigned short Swift_Number_bk=0;//当流水号一样就不存储数据
static int MAXLEN = 990 + 30;
twData tw_TANDA3016_Info;
application_info head=NULL;
char *Catch_Path = "./update/Catch_Tanda3016_232.bin";

extern volatile int ledFlag; 

//发送巡检
void Tanda_SendPolling(int fd)
{
	int i;
	unsigned char checksum=0;
	printf("\nsend poll:\n");
	memset(TxBuffer , 0 , TxBufferSize);
	tcflush(fd, TCOFLUSH); //清空发送缓存	
    TxBuffer[0] = '@';
    TxBuffer[1] = '@';	
    TxBuffer[2]=Swift_Number&0xff;//业务流水号低
    TxBuffer[3]=(Swift_Number>>8)&0xff;//业务流水号高
    TxBuffer[4] = MAIN_VERSION;
    TxBuffer[5] = SUB_VERSION;
	for(i = 0; i < 6; i++)  //fill dest addr
    {
        TxBuffer[18 + i] = RxBuffer[12+i];
    }
	TxBuffer[26]=CONTROL_OLDER_DATA;
	TxBuffer[27]=DOWN_TEST;
    TxBuffer[28] = 1; //信息对象数目
    TxBuffer[29] = 1; //设备类型 火灾报警器
    TxBuffer[30] = RxBuffer[12]; //设备地址 心跳包12位
    TxBuffer[31] = 1; //设备状态  
    TxBuffer[32] = 0; //状态发生时间 6字节
    TxBuffer[33] = 0; //
    TxBuffer[34] = 0; //
    TxBuffer[35] = 0; //
    TxBuffer[36] = 0; //
    TxBuffer[37] = 0; //

    TxBuffer[24] = 11; //应用数据单元长度低字节
    TxBuffer[25] = 0; //应用数据单元长度高字节

    for(i = 2; i< 38; i++)
    {
        TxBuffer[38] += TxBuffer[i];  //累加和
    }
    TxBuffer[39] = '#';
    TxBuffer[40] = '#';
	TxCounter=41;
    send_data(fd, TxBuffer, TxCounter);	
}
void Tanda_SendOlder(unsigned char *RxBuffer, int flag , int fd)
{
	
    int i;
	unsigned char checksum=0;	
	
	printf("\nsend older:\n");
	memset(TxBuffer , 0 , TxBufferSize);
	tcflush(fd, TCOFLUSH); //清空发送缓存
    MemCpy(TxBuffer , RxBuffer , 24);	
    TxBuffer[2]=RxBuffer[2];//业务流水号低
    TxBuffer[3]=RxBuffer[3];//业务流水号高
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
	printf("revdat:%d\n",revlen);

	if(revlen==0)
	{
		return -1;
	}
	for(j=0 ; j<28 ; j++)
	{
		printf("%x " , RxBuffer[j]);
	}
	printf("\n");
	for(j=28 ; j<revlen; j++)
	{
		printf("%x " , RxBuffer[j]);
	}
	printf("\n");
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
	return 0;
	
}

int Tanda_ReadDat(int fd1)
{
	printf("\nread tanda\n"); 
	memset(RxBuffer, 0, sizeof(RxBuffer));
	int ret=0, nread;
	fd_set fds;
	int count = 0;
    unsigned short len = 0; //calculate
	struct timeval tv;
	static unsigned char readmsg;
	while (1)
	{
		FD_ZERO(&fds);
		FD_SET(fd1, &fds);

		//timeout setting
		tv.tv_sec =  1;
		tv.tv_usec = 0;
		ret = select(fd1 + 1, &fds, NULL, NULL, &tv);
		if(ret < 0)
		{
			perror("select");
			ret=0;
			break;
		}
		else if(ret == 0)
		{
			printf("timeout\n");
			ret= 0;
			break;
		}

		if(FD_ISSET(fd1, &fds))
		{			
			readmsg=0;
			nread = read(fd1, &readmsg, 1);
			if(nread > 0)
			{
                ledFlag = 1;
				RxBuffer[count++] = readmsg;
				if(count > 25)
				{
                    len = (RxBuffer[24] + (RxBuffer[25]<<8)) + 30;
                    if(count >= len || count >= MAXLEN)
                        return count;
					//if(RxBuffer[count-2] == '#' && RxBuffer[count-1] == '#')
					//{
					//	return count;
					//}
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
    if(Swift_Number_bk == Swift_Number)
        return;
	switch(RxBuffer[27])
	{
        case DEVICE_STATE:  //TE1004   备电故障 类型标志为 1    add by ljh
        {
            if(infonum>19)
            {
                return FALSE;
            }
            for(i = 0; i < infonum; i++){
                tw_TANDA3016_Info.ControllerType = RxBuffer[29 + 9 * i];		//设备类型
                tw_TANDA3016_Info.ControllerID = RxBuffer[30 + 9 * i];		    //设备地址
                tw_TANDA3016_Info.componStatus = RxBuffer[31 + 9 * i];			//设备状态
                j = 0;
                for(j = 0; j < 6; j++)
                {
                    tw_TANDA3016_Info.occuTime[j] = RxBuffer[37 + 9 * i - j];
                }
                tw_TANDA3016_Info.servicetype = SET_CONTROLLER_STATE;
                if(tw_TANDA3016_Info.componStatus == 1)
                    insert_rxbuffer_data_head(head, tw_TANDA3016_Info);
                else
                    insert_rxbuffer_data(head, tw_TANDA3016_Info);
            }
            printf("..........XINHAOSI.........\n");
            break;
        }
		case PARTS_RUNNING_STATE:
		{			
			if(infonum>19)
			{
				return FALSE;
			}
			for(i=0 ; i<infonum ; i++)	
			{					
				tw_TANDA3016_Info.ControllerType=RxBuffer[29+50*i];
				tw_TANDA3016_Info.ControllerID=RxBuffer[30+50*i];
				tw_TANDA3016_Info.componType=RxBuffer[31+50*i];
				j=0;
				for(j=0 ; j<8 ; j++)
				{
					tw_TANDA3016_Info.componaddr[j]=RxBuffer[39+50*i-j];
				}
				tw_TANDA3016_Info.componStatus=RxBuffer[40+50*i];
				j=0;
				for(j=0 ; j<6 ; j++)
				{
					tw_TANDA3016_Info.occuTime[j]=RxBuffer[78+50*i-j];
				}
				tw_TANDA3016_Info.servicetype=SET_DEVICE_STATE;
                if(tw_TANDA3016_Info.componStatus == 1)
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
				tw_TANDA3016_Info.ControllerType=RxBuffer[29+10*i];		//设备类型
				tw_TANDA3016_Info.ControllerID=RxBuffer[30+10*i];		//设备地址
				tw_TANDA3016_Info.OperatorMark=RxBuffer[31+10*i];			//设备操作类型
				tw_TANDA3016_Info.OperatorPersonID=RxBuffer[32+10*i];	//操作员编号
				j=0;
				for(j=0 ; j<6 ; j++)
				{
					tw_TANDA3016_Info.occuTime[j]=RxBuffer[38+10*i-j];
				}
				tw_TANDA3016_Info.servicetype=SET_OPERATOR_INFO;
				insert_rxbuffer_data(head,tw_TANDA3016_Info);
			}
			break;
		}
		default:
			break;
	}
	return TRUE;
}
int Tanda_Get232_Data(int fd)
{
	int i;
	int ret;
	int revlen=0;
	static int pollcnt=0; //发送巡检需要按照协议4S
	memset(RxBuffer , 0 , RxBufferSize);
	revlen=Tanda_ReadDat(fd);	
    ledFlag = 0;
	
	//printf("\nreddat:%d 0x%2x 0x%2x 0x%2x\n",revlen , RxBuffer[26],RxBuffer[2],RxBuffer[3]);
	ret=TandaCheckDat(revlen);	
	//Swift_Number = RxBuffer[2]+(RxBuffer[3]<<8);
	if(ret==-1)			//should send polling older
	{	pollcnt++;
		if(pollcnt>10)		//大约1s
		{
			pollcnt=0;
			Tanda_SendPolling(fd);
		}
		return -1; //no date get
	}
	if(ret==-2)			//send reload older
	{	
		Tanda_SendOlder(RxBuffer, CONTROL_OLDER_NAK,fd);
		return -1; //no date get
	}
	
	Swift_Number = RxBuffer[2]+(RxBuffer[3]<<8);
	if(RxBuffer[26]==CONTROL_OLDER_DATA)		//发送数据
	{
		ret=Tanda_ProcessDat();
	    Swift_Number_bk = Swift_Number;
        usleep(50000);
		Tanda_SendOlder(RxBuffer, CONTROL_OLDER_ACK,fd);
	    Swift_Number++;
	}
	else if (RxBuffer[26]==CONTROL_OLDER_ACK)	//确认
	{
	}
	else if (RxBuffer[26]==CONTROL_OLDER_NAK)	//返回NAK 重传上一帧
	{		
		tcflush(fd, TCOFLUSH); //清空发送缓存
        usleep(50000);
		send_data(fd , TxBuffer , TxCounter);
	}

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
	int i=0 , recvlen=0;
	int ret=0;
	if(fd<0)
	{
		return 1;
	}
	if(port == PORT_RS232)
		ret=Tanda_Get232_Data(fd);
	else
		;//ret=Tanda_Get485_Data();
    return 1;
}

int tw_readRemoteController(int controllerid, int controllertype, int level, char *code,int readType)
{
	
}

int tw_setRemoteController(int controllerid, int controllertype, int level, char *code)
{
	
}


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
    unsigned char controllerID[8] = {0}, deviceStatus[8] = {0}, controllerType[5] = {0}, deviceType[5] = {0},  operatorMark[5] = {0}, operatornum[5] = {0}, deviceAddr[30] = {0};
	application_info app;
	app=get_rxbuffer_data(head);
	if(NULL == app)
	{
		get_catch_file( head);
		app=get_rxbuffer_data(head);
		if(NULL == app)
		{
			printf("list is empty");
			return NULL;
		}
	}
	
	ptr->ServerType=app->data.servicetype;
	ds = twDataShape_Create(twDataShapeEntry_Create("ControllerID", NULL, TW_STRING));
	if (!ds) 
	{
		TW_LOG(TW_ERROR, "Error Creating datashape.");
		return NULL;
	}
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("Flag", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerType", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceType", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("deviceNumber", NULL, TW_STRING));  //二次码方式
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceState", NULL, TW_STRING));

    if(ptr->ServerType == SET_OPERATOR_INFO)
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
	printf("\n\n\nthe board time is %s, now is%llu\n\n\n\n", now_time, now);
	/* populate the InfoTableRow with arbitrary data */
	itoa(app->data.ControllerID, controllerID);  //控制器类型
	itoa(app->data.componStatus, deviceStatus); //部件状态
	acciToStr(app->data.componaddr,deviceAddr,8);
	linuxTime = getConvertTime(app->data.occuTime);
	linuxTime*=1000;
	itoal(linuxTime, currentTime);
	
	itoa(app->data.ControllerType, controllerType);   //控制器类型
	itoa(app->data.componType, deviceType);       //部件类型
	TW_LOG(TW_DEBUG,"controllerType: %s, contorllerID: %s, currentTime : %s, deviceType: %s, deviceStatus: %s, deviceAddr: %s\n", controllerType, controllerID, currentTime, deviceType, deviceStatus, deviceAddr);
	row = twInfoTableRow_Create(twPrimitive_CreateFromString(controllerID, TRUE));
	if (!row) 
	{
		TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable row");
		twInfoTable_Delete(content);	
		return NULL;
	}
	if(app->data.servicetype == SET_OPERATOR_INFO || app->data.servicetype == SET_CONTROLLER_STATE)
	{
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("1", TRUE));             // FLAG  2:232, 3:485
    }
	else
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("2", TRUE));             // FLAG  2:232, 3:485
    twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(controllerType, TRUE));            //ControllerType
    twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceType, TRUE));           //DeviceType
    twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceAddr, TRUE));      //二次码
    twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceStatus, TRUE));            //state
    if(app->data.servicetype == SET_OPERATOR_INFO)
	{
        itoa(app->data.OperatorMark, operatorMark);
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(operatorMark, TRUE));    //操作类型
        itoa(app->data.OperatorPersonID, operatornum);
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(operatornum, TRUE));    //操作员编号
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
			TW_LOG(TW_TRACE, "*****************no date to save****************");
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
