#include "protocol.h"
#include "../../service.h"
#include <math.h>

#define X   0

static unsigned char RxBuffer[RxBufferSize] = {0};
static unsigned char TxBuffer[TxBufferSize] = {0};
static unsigned char rxbuf[RxBufferSize] = {0};
static unsigned int TxCounter = 0;
twData tw_Xinhaosi;
application_info head = NULL;
char * Catch_Path = "./update/Tanda3042B.bin";

extern volatile int ledFlag;
extern int controllerType;
extern int controllerID;
unsigned char loop = 0, channel = 0;
unsigned short addr;

unsigned short CRC16(unsigned char *puchMsg, unsigned short usDataLen);

//发送命令
void Tanda_SendCommand(int fd, unsigned char cmd, unsigned short addr, unsigned char len)
{
    unsigned short crc = 0;

	printf("\nsend command:");
	memset(TxBuffer, 0, TxBufferSize);
	tcflush(fd, TCOFLUSH); //清空发送缓存	

    TxBuffer[0] = (unsigned char)(controllerID / 10000000);
    TxBuffer[1] = cmd;	
    TxBuffer[2] = (unsigned char)(addr >> 8);
    TxBuffer[3] = (unsigned char)addr;
    TxBuffer[4] = 0x00;
    TxBuffer[5] = len;
    crc = CRC16(TxBuffer, 6);
    TxBuffer[6] = (unsigned char)(crc >> 8);
    TxBuffer[7] = (unsigned char)crc;

	TxCounter = 8;
    send_data(fd, TxBuffer, TxCounter);	
}

/*
 * 函  数： TandaCheckDat
 * 功  能： 校验数据 
 * 参  数： revlen  ——>     数据的长度（字节数）
 * 返回值： 成功    ——>     0
 *          长度为0 ——>     -1
 *          错误    ——>     -2
 * 描  述： 检查RxBuffer中数据长度、启动符和校验和是否正确
 *          被Tanda_Get485_Data调用
 * 时  间： 2016-10-14
 */
int TandaCheckDat(int revlen)
{
	int i;
	unsigned char checksum=0;
    unsigned short crcsum=0;
	if (revlen == 0)
		return -1;
    
	if (RxBuffer[2] != revlen - 4) {
		printf("\n Data len err\n");
		return -2;
	}
	crcsum = (RxBuffer[revlen - 2] << 8) + RxBuffer[revlen - 1];
    if(crcsum != CRC16(RxBuffer, revlen - 2))
    {
        printf("\n------CRC err------\n");
        return -3;
    }
    return 0;
}

/*
 * 函  数： Tanda_ReadDat
 * 功  能： 读取数据 
 * 参  数： fd1     ——>     串口设备文件描述符
 * 返回值： 成功    ——>     读取到的字节数
 *          失败    ——>     -1
 *          超时    ——>     0
 * 描  述： 打开串口设备文件，循环读取数据存入RxBuffer中
 *          被Tanda_Get485_Data调用
 * 时  间： 2016-10-14
 */
int Tanda_ReadDat(int fd1, unsigned char *buff)
{
    int ret = 0;
	int revlen = 0;
	fd_set fds;
    int rxflag = 0;
	struct timeval tv;
	static unsigned char readmsg;

	printf("\nread tanda:\n"); 
    memset(buff, 0, RxBufferSize);

    while (1) {
        FD_ZERO(&fds);
        FD_SET(fd1, &fds);

        //timeout setting
        tv.tv_sec =  1;
        tv.tv_usec = 0;

        if ((ret = select(fd1 + 1, &fds, NULL, NULL, &tv)) < 0) {
            perror("select");
            return -1;
        }

        if (ret == 0) {
            printf("timeout\n");
            break;
        }

        if (FD_ISSET(fd1, &fds)) {			
            readmsg = 0;
            if (read(fd1, &readmsg, 1) > 0) {
                ledFlag = 1; //led 闪烁
                buff[revlen++] = readmsg;
            }
        }
    }

    return revlen;
}

/*
 * 函  数： Tanda_ProcessDat
 * 功  能： 处理数据
 * 参  数： void
 * 返回值： 成功    ——>     TRUE
 *          失败    ——>     FALSE
 * 描  述： 把RxBuffer中的数据封装成结构体并插入到链表中
 *          被Tanda_Get485_Data调用
 * 时  间： 2016-10-14
 */
int Tanda_ProcessDat(void)
{
    int index = 0;
    int sign = 0;
    int tmp = 0;
    float n = 0;
    unsigned short y = 0;
    int i, j;
    unsigned char x = 0;
    static float backup[10000] = {0};
    static unsigned char state1[6][32] = {0};
    static unsigned char state2;
    static unsigned char status[] = {23, 22, 132, 43, 42, 133};
    static unsigned char state3[10000] = {0};

    if(0 == RxBuffer[2])
        return FALSE;

    memset(&tw_Xinhaosi, 0, sizeof(tw_Xinhaosi));
    tw_Xinhaosi.deviceType = controllerType;
    tw_Xinhaosi.deviceAddr = controllerID / 10000000;

    switch (RxBuffer[1]) {
        case 0x01:
            tw_Xinhaosi.componLoopID = loop;
            tw_Xinhaosi.servicetype = SET_DEVICE_STATE;
            for (i = 0; i < RxBuffer[2]; i++) {
                if ((x = RxBuffer[3 + i] ^ state1[loop][i]) == 0)
                    continue;
                state1[loop][i] = RxBuffer[3 + i];

                for (j = 0; j < 8; j++) {
                    if ((x >> 1) << 1 != x) {
                        tw_Xinhaosi.componAddr = i * 8 + j;
                        if ((RxBuffer[3 + i] >> j) & 0x01)
                            tw_Xinhaosi.componStatus = 60;              /* 启动 */
                        else
                            tw_Xinhaosi.componStatus = 70;
                        insert_rxbuffer_data(head, tw_Xinhaosi);
                    }
                    x >>= 1;
                }
            }
            break;

        case 0x02:
            tw_Xinhaosi.servicetype = SET_CONTROLLER_STATE;
            if ((x = RxBuffer[3] ^ state2) == 0)
                break;
            state2 = RxBuffer[3];

            for (i = 0; i < 3; i++) {
                if ((x >> 1) << 1 != x) {
                    if (state2 >> i & 0x01) {
                        tw_Xinhaosi.componStatus = status[i];
                    } else {
                        tw_Xinhaosi.componStatus = status[3 + i];
                    }
                    insert_rxbuffer_data(head, tw_Xinhaosi);
                }
                x >>= 1;
            }
            break;

        case 0x03:
            tw_Xinhaosi.componLoopID = loop;
            tw_Xinhaosi.componChannel = channel + 1;
            tw_Xinhaosi.servicetype = SET_DEVICE_STATE;
            for (i = 0; i < 200; i += 2) {
                tmp = RxBuffer[4 + i];

                /*
                 * 如果节点不存在，跳过
                 */
                if (tmp == 7)
                    continue;

                y = rxbuf[3 + i] << 8 | rxbuf[4 + i];
                sign = y >> 15;
                index = y >> 13 & 3;
                tw_Xinhaosi.AnalogValue = (float)(y & 0x1FFF) * pow(10, index); //模拟量的值
                if (sign)
                    tw_Xinhaosi.AnalogValue = -1 * tw_Xinhaosi.AnalogValue;
                printf("********************************************************************************>> %f\n", tw_Xinhaosi.AnalogValue);
                j = channel << 12 | loop << 8 | addr + i / 2; 
                n = backup[j];
                backup[j] = tw_Xinhaosi.AnalogValue; //备份本次模拟量

                if (tmp == state3[j]) {
                    if (fabsf(tw_Xinhaosi.AnalogValue - n) < 1) 
                        continue;
                    if (tmp == 10)
                        tw_Xinhaosi.componStatus = 0;
                    else if (tmp == 15)
                        tw_Xinhaosi.componStatus = 3;
                    else if (tmp == 14)
                        tw_Xinhaosi.componStatus = 2;
                    else if (tmp == 13)
                        tw_Xinhaosi.componStatus = 12;
                    else if (tmp == 1)
                        tw_Xinhaosi.componStatus = 21;
                    else
                        tw_Xinhaosi.componStatus = 20;
                } else {
                    if (tmp == 10) {
                        if (state3[j] == 1)
                            tw_Xinhaosi.componStatus = 41;
                        else if (state3[j] == 2)
                            tw_Xinhaosi.componStatus = 40;
                        else if (state3[j] == 0)
                            tw_Xinhaosi.componStatus = 0;
                        else
                            tw_Xinhaosi.componStatus = 13;
                    } else if (tmp == 15)
                        tw_Xinhaosi.componStatus = 3;
                    else if (tmp == 14)
                        tw_Xinhaosi.componStatus = 2;
                    else if (tmp == 13)
                        tw_Xinhaosi.componStatus = 12;
                    else if (tmp == 1)
                        tw_Xinhaosi.componStatus = 21;
                    else
                        tw_Xinhaosi.componStatus = 20;

                    state3[j] = tmp; //备份本次状态

                    insert_rxbuffer_data_head(head, tw_Xinhaosi); //如果状态变化，插入头，马上上传
                    break;
                }

                tw_Xinhaosi.componAddr = addr + i / 2;
                if(tw_Xinhaosi.componStatus != 0)
                    insert_rxbuffer_data_head(head, tw_Xinhaosi);
                else
                    insert_rxbuffer_data(head,tw_Xinhaosi);
            }
            break;

        default:
            break;
    }

    return TRUE;
}

/*
 * 函  数： Tanda_Get485_Data
 * 功  能： 获取数据存入链表
 * 参  数： fd      ——>     串口设备文件描述符
 * 返回值： 成功    ——>     0
 *          失败    ——>     -1
 * 描  述： 设置485流控
 *          调用Tanda_ReadDat读数据
 *          调用TandaCheckDat校验数据
 *          调用Tanda_ProcessDat处理数据
 * 时  间： 2016-10-14
 */
int Tanda_Get232_Data(int fd)
{
	int ret, i;
	int revlen = 0;
    static unsigned char max_loop = 5;
    static unsigned char max_channel = 8;
    static unsigned char max_addr = 200;

    if (loop == 0) {
        max_addr = controllerID % 1000;
        max_channel = controllerID / 1000 % 100;
        max_loop = controllerID / 100000 % 100;
    }

    if (channel == 0) {
        loop++;
        if (loop > max_loop)
            loop = 1;

        /*
         * 读取外控开关状态
         */
        /*
        Tanda_SendCommand(fd, 0x01, 0, 0xFF);
        if ((revlen = Tanda_ReadDat(fd, RxBuffer)) > 0) {
            printf("revlen: %d\n", revlen);
#if X
            for (i = 0; i < revlen; i++)
                printf("%02X ", RxBuffer[i]);
            putchar('\n');
#endif

            ledFlag = 0; //led 停止闪烁
            Tanda_ProcessDat();
        }
        */
        Tanda_SendCommand(fd, 0x01, loop << 8, 0xFF);
        if ((revlen = Tanda_ReadDat(fd, RxBuffer)) > 0) {
            printf("revlen: %d\n", revlen);
#if X
            for (i = 0; i < revlen; i++)
                printf("%02X ", RxBuffer[i]);
            putchar('\n');
#endif

            ledFlag = 0; //led 停止闪烁
            Tanda_ProcessDat();
        }
    }

    if (loop == 1 && channel == 0) {
        /*
         * 读取控制器状态
         */
        Tanda_SendCommand(fd, 0x02, 0x0001, 4);
        if ((revlen = Tanda_ReadDat(fd, RxBuffer)) > 0) {
            printf("revlen: %d\n", revlen);
#if X
            for (i = 0; i < revlen; i++)
                printf("%02X ", RxBuffer[i]);
            putchar('\n');
#endif

            ledFlag = 0; //led 停止闪烁
            Tanda_ProcessDat();
        }
    }

    if (max_addr > 200)
        max_addr = 200;
    /*
     * 读取探测器状态
     */
    for (addr = 1; addr <= max_addr; addr += 100) {
        Tanda_SendCommand(fd, 0x04, channel << 12 | loop << 8 | addr, 100);
        if ((revlen = Tanda_ReadDat(fd, rxbuf)) > 0) {
            printf("revlen: %d\n", revlen);
#if X
            for (i = 0; i < revlen; i++)
                printf("%02X ", rxbuf[i]);
            putchar('\n');
#endif

            ledFlag = 0; //led 停止闪烁
        }

        Tanda_SendCommand(fd, 0x03, channel << 12 | loop << 8 | addr, 100);
        if ((revlen = Tanda_ReadDat(fd, RxBuffer)) > 0) {
            printf("revlen: %d\n", revlen);
#if X
            for (i = 0; i < revlen; i++)
                printf("%02X ", RxBuffer[i]);
            putchar('\n');
#endif

            ledFlag = 0; //led 停止闪烁
            Tanda_ProcessDat();
        }
    }

    channel++;
    if (channel >= max_channel)
        channel = 0;

    return 0;
}

/*
 * 函  数： tw_InitList
 * 功  能： 初始化数据缓冲链表
 * 参  数： fd      ——>     串口设备文件描述符
 * 返回值： 成功    ——>     0
 *          失败    ——>     -1
 * 时  间： 2016-10-14
 */
int tw_InitList(int fd)
{	
	head = init_rxbuffer_data_list();

	if (head == NULL)
		return -1;

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

/*
 * 函  数： tw_GetUARTInformation
 * 功  能： 获取控制器上传的数据
 * 参  数： fd      ——>     串口设备文件描述符
 *          port    ——>     串口类型
 * 返回值： 成功    ——>     0
 *          失败    ——>     -1
 * 描  述： 判断串口类型，调用Tanda_Get485_Data获取数据
 * 时  间： 2016-10-14
 */
int tw_GetUARTInformation(int fd , int port)
{
	int i = 0, recvlen=0;
	int ret = 0;

	if (fd < 0)
		return 1;

	if (port == PORT_RS232)
        ret = Tanda_Get232_Data(fd);
    else
		;//ret = Tanda_Get485_Data(fd);

    return 0;
}

int tw_readRemoteController(int controllerid, int controllertype, int level, char *code,int readType)
{
	
}

int tw_setRemoteController(int controllerid, int controllertype, int level, char *code)
{
	
}

twInfoTable* tw_CreateUARTInfoTable(DATETIME now, int Num, tw_Public *ptr)
{
	/* Make the request to the server */
	
    int code;
	twInfoTable * content = NULL;
	twInfoTableRow *row = 0;
	twInfoTable *it = NULL;
	twDataShape * tempDS = NULL;
	twDataShape * ds = 0;
	char now_time[25];
	long long linuxTime = 0;
	int ret;
    char currentTime[25] = {0};
    unsigned char controllerID[8] = {0},
                  deviceStatus[8] = {0},
                  controllerType[5] = {0},
                  deviceType[5] = {0},
                  operatorMark[5] = {0},
                  operatornum[5] = {0},
                  deviceAddr[30] = {0},
                  deviceLoopID[8] = {0},
                  deviceChannel[8] = {0};
	application_info app;

    app=get_rxbuffer_data(head);
    if(NULL == app)
    {
        get_catch_file( head);
        app = get_rxbuffer_data(head);
        if(NULL == app)
        {
            printf("list is empty");
            return NULL;
        }
    }

    itoal(now, now_time);	
    printf("\n\n\nthe board time is %s, now is%llu\n\n\n\n", now_time, now);

    if (app->data.componStatus == 0 || app->data.componStatus == 3 || app->data.componStatus == 2 || app->data.componStatus == 12) {
        printf("###############################################################################################################%d\n", app->data.componStatus);
        printf("###############################################################################################################%f\n", app->data.AnalogValue);
        ds = twDataShape_Create(twDataShapeEntry_Create("ControllerId", NULL, TW_INTEGER));
        if (!ds) { 
            TW_LOG(TW_ERROR, "Error Creating datashape.");
            return NULL;
        }
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerType", NULL, TW_INTEGER));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("LoopId", NULL, TW_INTEGER));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceAddr", NULL, TW_INTEGER));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("ChannelId", NULL, TW_INTEGER));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("CollectTime", NULL, TW_DATETIME));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("AnalogValueType", NULL, TW_INTEGER));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("AnalogValue", NULL, TW_NUMBER));

        content = twInfoTable_Create(ds);
        if (!content) { 
            TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable");
            twDataShape_Delete(ds); 
            return NULL;
        }

        row = twInfoTableRow_Create(twPrimitive_CreateFromInteger(app->data.deviceAddr));
        if (!row) { 
            TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable row");
            twInfoTable_Delete(content);	
            return NULL;
        }
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromInteger(app->data.deviceType));
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromInteger(app->data.componLoopID));
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromInteger(app->data.componAddr));
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromInteger(app->data.componChannel));
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromDatetime(now));
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromInteger(app->data.componStatus));
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromNumber(app->data.AnalogValue));

        twInfoTable_AddRow(content, row);

        tempDS = twDataShape_Create(twDataShapeEntry_Create("AnalogValue", NULL, TW_INFOTABLE));
        twInfoTable* tempInfotable = twInfoTable_Create(tempDS);

        twPrimitive *twStatusInfotable = twPrimitive_CreateFromInfoTable(content);
        twInfoTableRow *InfoTableRow = twInfoTableRow_Create(twStatusInfotable);
        twInfoTable_AddRow(tempInfotable, InfoTableRow);

        code = twApi_InvokeService(TW_THING, AdaptorID, "UploadCollectCardChannelAnalog", tempInfotable, &it, -1, TRUE); //success return 0  调用云平台函数
        if (code) 
        {
            TW_LOG(TW_ERROR, "Error invoking service on Platform. EntityName: %s ServiceName: %s", AdaptorID, "UploadCollectCardChannelAnalog");			
        }

        TW_LOG(TW_DEBUG, "*****************Sync Data Analog*****************");
        printf("###########################################################################################################################SUCCESS\n");
        tw_Delete_CurrentNode();

        return NULL;
    } else {

        ptr->ServerType = app->data.servicetype;
        ds = twDataShape_Create(twDataShapeEntry_Create("ControllerID", NULL, TW_STRING));
        if (!ds) { 
            TW_LOG(TW_ERROR, "Error Creating datashape.");
            return NULL;
        }
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("Flag", NULL, TW_STRING));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerType", NULL, TW_STRING));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceLoopID", NULL, TW_STRING));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceType", NULL, TW_STRING));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceAddr", NULL, TW_STRING));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("ChannelID", NULL, TW_STRING));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("CurrentTime", NULL, TW_STRING));
        twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceState", NULL, TW_STRING));

        content = twInfoTable_Create(ds);
        if (!content) { 
            TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable");
            twDataShape_Delete(ds); 
            return NULL;
        }

        /* populate the InfoTableRow with arbitrary data */
        itoa(app->data.deviceAddr, controllerID);
        itoa(app->data.deviceType, controllerType);   //控制器类型
        itoa(app->data.componLoopID, deviceLoopID);
        itoa(app->data.componType, deviceType);       //部件类型
        itoa(app->data.componAddr, deviceAddr);
        itoa(app->data.componChannel, deviceChannel);
        itoa(app->data.componStatus, deviceStatus); //部件状态

        row = twInfoTableRow_Create(twPrimitive_CreateFromString(controllerID, TRUE));
        if (!row) { 
            TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable row");
            twInfoTable_Delete(content);	
            return NULL;
        }
        if (ptr->ServerType == SET_CONTROLLER_STATE)
            twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("1", TRUE));             // FLAG  2:232, 3:485
        else
            twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("3", TRUE));             // FLAG  2:232, 3:485
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(controllerType, TRUE));
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceLoopID, TRUE));
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceType, TRUE));
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceAddr, TRUE));
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceChannel, TRUE));
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(now_time, TRUE));        //time
        twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceStatus, TRUE));

        TW_LOG(TW_DEBUG,"controllerType: %s, contorllerID: %s, currentTime: %s, deviceType: %s, deviceStatus: %s, deviceAddr: %s, operatorMark: %s\n",
                controllerType, controllerID, currentTime, deviceType, deviceStatus, deviceAddr, operatorMark);

        /* add the InfoTableRow to the InfoTable */
        twInfoTable_AddRow(content, row);

        return content;
    }
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

static const unsigned char auchCRCHi[] = 
{
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
	0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40
} ;
/* Table of CRC values for low–order byte */
static const unsigned char auchCRCLo[] = 
{
	0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
	0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
	0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
	0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
	0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
	0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
	0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
	0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
	0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
	0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
	0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
	0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
	0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
	0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
	0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
	0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
	0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
	0x40
} ;

/**
 * 快速CRC16校验算法(CRC-16/IBM ) modbus
 * 多项式: x16+x15+x2+1
 * @param  puchMsg     缓冲区指针
 * @param  usDataLen 数据体长度
 * @return         CRC16校验值
 */
unsigned short CRC16(unsigned char *puchMsg, unsigned short usDataLen)
{
	unsigned char uchCRCHi = 0xFF ; /* 初始化高字节*/
	unsigned char uchCRCLo = 0xFF; /* 初始化低字节*/
	unsigned short uIndex;     /*把CRC表*/
	unsigned char Temp;

	while(usDataLen--)     /*通过数据缓冲器*/
	{
		Temp = *puchMsg++;
		
	   uIndex = uchCRCHi ^ Temp ; /*计算CRC */
	   uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex] ;
	   uchCRCLo = auchCRCLo[uIndex] ;
	}
	return (uchCRCHi << 8 | uchCRCLo) ;
}
