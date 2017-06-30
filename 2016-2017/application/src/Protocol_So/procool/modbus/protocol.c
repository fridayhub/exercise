#include "protocol.h"
#include "../../../service.h"
static u8 RxBuffer[RxBufferSize] = {0};
static u8 TxBuffer[TxBufferSize] = {0};
static unsigned int TxCounter = 0;

extern volatile int ledFlag;
extern char AdaptorID[100];

waterData tw_water_Info;
application_info head = NULL;
char * Catch_Path = "./update/Tanda_modbus.bin";

u16 usMBCRC16(u8 *srcMsg, u16 dataLen);

double watersql[16][8];
char waterlocation[16][100];
int row = 0; //record row
int hasreadsql = 0;
int UpConfig = 0; //上传配置标志  只上传一次

int type, rate = 1; // 传感器类型， 上传速率
int uplimit, lowlimit;  //上限值， 下限值
void updateLocalSetting(void);
double oldvalue[16];

void tw_Delete_CurrentNode(void)
{
	delete_rxbuffer_data(head);
}

/* callback函数中： 
 * void*,  Data provided in the 4th argument of sqlite3_exec()
 * int,    The number of columns in row 
 * char**, An array of strings representing fields in the row
 * char**  An array of strings representing column names
 */  
int callback(void *data, int nr, char **values, char **name)
{
    int i;
    //fprintf(stderr, "%s:\n", (const char *)data);
    for(i = 0; i < (nr - 1); i++){
        watersql[row][i] = strtod(values[i], NULL);
        //printf("%s = %s", name[i], atoi(values[i]));
    }
    strcpy(waterlocation[row], values[i]);
    row++;
    return 0; 
}

int ReadSqlite(void)
{
    char *SelectSql;
    sqlite3 *db;
    int rc;
    char *zErrMsg = 0;
    const char * data = "Callback function called";

    rc = sqlite3_open("/data/.watersys.db", &db); //open or create a db
    if( rc ){
        fprintf(stderr, "Can't open databases: %s\n", sqlite3_errmsg(db));
        return -1;
    }else{
        fprintf(stdout, "OPened database successfully\n");
    }

    /* select sql statement */
    SelectSql = "select * from watersys;";
    rc = sqlite3_exec(db, SelectSql, callback, (void*)data, &zErrMsg);
    if( rc ){
        fprintf(stderr, "SQL select error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -1;
    }else{
        fprintf(stdout, "select execd successfully\n");
    }
    sqlite3_close(db);
    hasreadsql = 1; //only read once
    return 1;
}

int updateCallback(void *data, int nr, char **values, char **name)
{
    TW_LOG(TW_TRACE, "SQLITE3 CALLBACK FUNCTION");
    return 0; 
}

int WriteSqlite(int channel, double uplimite, double lowlimite, double rate)
{
    char WriteSql[1024];
    sqlite3 *db;
    int rc;
    char *zErrMsg = 0;
    const char * data = "write Sqlite callback called";

    rc = sqlite3_open("/data/.watersys.db", &db); //open or create a db
    if( rc ){
        fprintf(stderr, "Can't open databases: %s while write\n", sqlite3_errmsg(db));
        return -1;
    }else{
        fprintf(stdout, "OPened database successfully while write\n");
    }

    /*write sql statement*/
    sprintf(WriteSql, "UPDATE watersys SET uplimite=%lf, lowlimite=%lf, rate=%lf WHERE channel=%d;", uplimite, lowlimite, rate, channel);
    printf("exec:%s", WriteSql);
    rc = sqlite3_exec(db, WriteSql, updateCallback, (void*)data, &zErrMsg);
    if( rc ){
        fprintf(stderr, "SQL update error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -1;
    }else{
        fprintf(stdout, "update execd successfully\n");
    }
    sqlite3_close(db);
    hasreadsql = 0; //only read once

    return 1;
}


/* funcod： 功能码
 * num ： 要查询模拟量数目
 * startaddr： 起始地址
 *
 * */
void SendCmd(int fd, u8 funcod, u16 num, u16 startaddr){
    u16 usCRC16;
   memset(TxBuffer, 0, TxBufferSize); 
   TxBuffer[0] = 0xFE; //暂时固定为广播地址
   TxBuffer[1] = funcod; //指令码
   TxBuffer[2] = (startaddr >> 8); //高8位
   TxBuffer[3] = (startaddr & 0xFF); //低8位
   TxBuffer[4] = (num>> 8); //高8位
   TxBuffer[5] = (num & 0xFF); //低8位
   usCRC16 = usMBCRC16(TxBuffer, 6);
   TxBuffer[6] = (usCRC16 >> 8) & 0xFF;
   TxBuffer[7] = usCRC16 & 0xFF;
   //TxBuffer[8] = 0x00;

   send_data(fd, TxBuffer, 8);
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
int Tanda_ReadDat(int fd1, u16 num)
{
	int ret = 0;
	int count = 0;
	fd_set fds;
	struct timeval tv;
	static u8 readmsg;
    u16 datalen = num * 2 + 5;

	printf("\nread tanda\n"); 
	memset(RxBuffer, 0, RxBufferSize);

    while (1) {
        FD_ZERO(&fds);
        FD_SET(fd1, &fds);

        //timeout setting
        tv.tv_sec =  2;
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
            if (read(fd1, &readmsg, 1) > 0){
                ledFlag = 1; //led 闪烁
                if(count == 0 && readmsg == 0)
                    continue;
                else
                    RxBuffer[count++] = readmsg;
            }
            //printf("Get data: %x\n", readmsg); 
            if (count > 1) {
				if(RxBuffer[0] == 0xFE && count >= datalen)
                    return count;
            }
        }
    }

    return ret;
}

/*
 * range: 量程
 * value: 读到的模拟量
 * startvalue 初始电流值 从网页获取
*/
double convert(double range, unsigned short value, double startvalue, int type)
{
   int base = 4000;
   double result;
   printf("range:%lf, value:%u\n", range, value);
   if(value < 2000)
       return 0.1;  //断线传0
   if(value > 2000 && value < 4000)
       return 0;
   if(type == 1) //水位 cm
       result = (((double)value - startvalue) * (range * 100 / 16000));
   else if(type == 0) //压力 pa
       result = (((double)value - startvalue) * (range * 1000000 / 16000));
   printf("comput result:%lf\n", result);
   return result;
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
	int i,j;
    int ret;
    double result;
    unsigned short rd_tmp;
	
	int infonum=RxBuffer[28];
    if(hasreadsql == 0){
        ret = ReadSqlite();
        if(!ret){
            fprintf(stderr, "get data from sqlite error"); 
            hasreadsql = 0;
        }
        updateLocalSetting();
    }
    for(i = 0; i < 16; i++){
        if(watersql[i][2] == 1){ //筛选勾选项
            for(j = 0; j < 8; j++){
                printf("%lf ", watersql[i][j]);
            }
            printf("location:%s\n", waterlocation[i]);
            rd_tmp = (RxBuffer[3+i*2]<<8) + RxBuffer[3+i*2+1]; //读到的模拟量
            printf("rd_tmp:%d\n", rd_tmp);
            //result = convert(watersql[i][3], rd_tmp, watersql[i][7]);

            type = (int)watersql[i][1]; //数据类型  压力  液位高度

            result = convert(watersql[i][3], rd_tmp, 4000, type); //此值暂时写为说明书初始值
            printf("result:%lf\n", result);
            tw_water_Info.analogValue = result; //模拟量值
            tw_water_Info.channelValue = i; //通道号
            tw_water_Info.servicetype = WATER_UPLOAD;
            insert_rxbuffer_data(head, tw_water_Info); //插入链表

            uplimit = (int)(watersql[i][4] * 100);
            lowlimit = (int)(watersql[i][5] * 100);
            rate =  (int)watersql[i][6];
            printf("uplimit %d, lowlimit %d\n", uplimit, lowlimit);

            if(result > lowlimit && result < uplimit){ //convert to cm
                TW_LOG(TW_TRACE, "Sleep rate is %d\n", rate);
                sleep(rate); //休眠相应时间
            }

            if(UploadData())
               tw_Delete_CurrentNode(); //上传数据 成功后删除这个节点
        }
        printf("\n");
    }

    row = 0; // set 0 when data processed

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
 *          调用usMBCRC16校验数据
 *          调用Tanda_ProcessDat处理数据
 * 时  间： 2016-12-20
 */
int Tanda_Get485_Data(int fd)
{
	int ret, i;
	int revlen = 0;
    u16 num = 0; //查询模拟量数量 

    usleep(80000);
    ret=Set_485TX_Enable(fd);
    if(ret == -1){
        perror("Set MBWater to Tx faild!");
        return -1;
    }

    //usleep(80000);
    tcflush(fd, TCIOFLUSH);
    num = 16;
    SendCmd(fd, ReadInReg, num, 0);

    usleep(8400);
	ret = Set_485RX_Enable(fd);
	if(ret == -1) {
        return -1;
        perror("Error config 485 to read");
    }

    revlen = Tanda_ReadDat(fd, num);
    ledFlag = 0; //led 停止闪烁

    for (i = 0; i < revlen; i++)
        printf("%02X ", RxBuffer[i]);
    putchar('\n');

    /* Length and CRC check*/
    if((revlen >= 4) && (revlen <=256)){
        if(usMBCRC16(RxBuffer, revlen) != 0){
            TW_LOG(TW_ERROR, "Get data error!");
            return 0;
        }
    }
    printf("Read value is :%d\n", ((RxBuffer[33]<<8) + RxBuffer[34]));

    ret = Tanda_ProcessDat(); //处理数据 添加到链表 

    ///////////////
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



/*
 * 函  数： tw_GetUARTInformation
 * 功  能： 获取控制器上传的数据
 * 参  数： fd      ——>     串口设备文件描述符
 *          port    ——>     串口类型
 * 返回值： 成功    ——>     0
 *          失败    ——>     -1
 * 描  述： 判断串口类型，调用Tanda_Get485_Data获取数据
 * 时  间： 2016-12-20
 */
int tw_GetUARTInformation(int fd , int port)
{
	int i = 0, recvlen=0;
	int ret = 0;

	if (fd < 0)
		return 1;

	if (port == PORT_RS485)
		ret = Tanda_Get485_Data(fd);

    return 0;
}

int tw_readRemoteController(int controllerid, int controllertype, int level, char *code,int readType)
{
	
}

int tw_setRemoteController(int controllerid, int controllertype, int level, char *code)
{
	
}

/*water remote service*/
enum msgCodeEnum remoteWtHandler(const char * entityName, const char * serviceName, twInfoTable * params, twInfoTable ** content, void * userdata)
{
    TW_LOG(TW_TRACE, "water sys remoteHandler -function called %s", serviceName);
    if(!content)
    {
        TW_LOG(TW_ERROR, "remoteHandler- NULL content pointer");
        return TWX_BAD_REQUEST;
    }
    twPrimitive * getparams;
    int  channel, signaltype, ret;
    double range, uplimite, lowlimite, rate;
    if(strcmp(entityName, AdaptorID) == 0){
        if(strcmp(serviceName, "SetCollectCardConfig") == 0){
            twInfoTable_GetPrimitive(params, "ConfigInfo", 0, &getparams); //先得到配置表
            twInfoTable_GetInteger(getparams->val.infotable, "Channel", 0, &channel);
            twInfoTable_GetNumber(getparams->val.infotable, "HighAlarmValue", 0, &uplimite); 
            twInfoTable_GetNumber(getparams->val.infotable, "LowAlarmValue", 0, &lowlimite); 
            twInfoTable_GetNumber(getparams->val.infotable, "CollectPeriod", 0, &rate); 
            TW_LOG(TW_TRACE, "get water sys config from remote successful");

            /* write to database */
            TW_LOG(TW_TRACE, "Remote data: %d, %lf, %lf, %lf", channel, uplimite, lowlimite, rate);
            TW_LOG(TW_TRACE, "Write data to database");
            WriteSqlite(channel, uplimite, lowlimite, rate);
        }
    }
    return TWX_SUCCESS;
}

void updateLocalSetting(void)
{
    twDataShape *ds = 0;
    ds = NULL;

    ds = twDataShape_Create(twDataShapeEntry_Create("Channel", NULL, TW_INTEGER));
    if(!ds){
        TW_LOG(TW_ERROR, "Error Creating datashape for getDeviceState");
        exit(1);
    }
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("HighAlarmValue", NULL, TW_NUMBER));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("LowAlarmValue", NULL, TW_NUMBER));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("CollectPeriod", NULL, TW_NUMBER));

    twApi_RegisterService(TW_THING, AdaptorID, "SetCollectCardConfig", NULL, ds, TW_BOOLEAN, NULL, remoteWtHandler, NULL);
}

int configTableInit(twInfoTable * content)
{
    int code = 0;
    twInfoTable * it = NULL;
    twDataShape * configDS = 0;
    configDS = twDataShape_Create(twDataShapeEntry_Create("ConfigInfo", NULL, TW_INFOTABLE));
    twInfoTable * waterconfig = twInfoTable_Create(configDS);

    twPrimitive * twConfigInfotable = twPrimitive_CreateFromInfoTable(content);
    twInfoTableRow *InfotableRow = twInfoTableRow_Create(twConfigInfotable);
    twInfoTable_AddRow(waterconfig, InfotableRow);

	code = twApi_InvokeService(TW_THING, AdaptorID, "UpdateCollectCardConfig", waterconfig, &it, -1, TRUE); //success return 0  调用云平台函数
    if(code){
        TW_LOG(TW_ERROR, "Error invoking service on Platform. EntityName: %s Service Name: %s", AdaptorID, "UpdateCollectCardConfig");
        return -1;
    }
    return 1;
}

twInfoTable * uploadSetting(DATETIME now)
{
	/* Make the request to the server */
	
	twInfoTable * content = NULL;
	twInfoTableRow *row = 0;
	twDataShape * ds = 0;
	char now_time[25];
	int ret;
    int i;

	ds = twDataShape_Create(twDataShapeEntry_Create("Channel", NULL, TW_INTEGER));
	if (!ds) 
	{
		TW_LOG(TW_ERROR, "Error Creating datashape.");
		return NULL;
	}
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("SignalType", NULL, TW_NUMBER));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("HighAlarmValue", NULL, TW_NUMBER));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("LowAlarmValue", NULL, TW_NUMBER));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("CollectPeriod", NULL, TW_NUMBER)); //采集周期
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("CollectPlace", NULL, TW_STRING));
	
	content = twInfoTable_Create(ds);
	if (!content) 
	{
		TW_LOG(TW_ERROR, "Watersystem - Error creating infotable");
		twDataShape_Delete(ds); 
		return NULL;
	}

	
	printf("\n\n\nthe board time is %s, now is%llu\n\n\n\n", now_time, now);
	/* populate the InfoTableRow with arbitrary data */

    for(i = 0; i < 16; i++){
        if(watersql[i][2] == 1){ //筛选勾选项
           twPrimitive *twChannel = twPrimitive_CreateFromInteger(watersql[i][0] + 1);
          twInfoTableRow *InfoTableRow = twInfoTableRow_Create(twChannel); 

          twPrimitive *twSignalType = twPrimitive_CreateFromNumber(watersql[i][1]); //信号类型
          twInfoTableRow_AddEntry(InfoTableRow, twSignalType);

          twPrimitive *twHighAlarmValue = twPrimitive_CreateFromNumber(watersql[i][4]); //报警上限
          twInfoTableRow_AddEntry(InfoTableRow, twHighAlarmValue);
          
          twPrimitive *twLowAlarmValue = twPrimitive_CreateFromNumber(watersql[i][5]); //报警下限
          twInfoTableRow_AddEntry(InfoTableRow, twLowAlarmValue);

          twPrimitive *twCollectPeriod= twPrimitive_CreateFromNumber(watersql[i][6]);  //采集周期
          twInfoTableRow_AddEntry(InfoTableRow, twCollectPeriod);
          
          twPrimitive *twlocation= twPrimitive_CreateFromString(waterlocation[i], TRUE); //采集位置
          twInfoTableRow_AddEntry(InfoTableRow, twlocation);

            /* add the InfoTableRow to the InfoTable */
            twInfoTable_AddRow(content, InfoTableRow);
        }

    }
        return content;
}

int UploadData(void )
{

    twInfoTable * content = NULL;
    twInfoTable * it = NULL;
    twInfoTableRow *row = 0;
    twDataShape * ds = 0;
    int ret, code, i = 0;
    application_info app;
	DATETIME now = twGetSystemTime(TRUE);
    app=get_rxbuffer_data(head);
    if(NULL == app)
    {
        get_catch_file( head);
        app=get_rxbuffer_data(head);
        if(NULL == app)
        {
            printf("list is empty");
            return -1;
        }
    }

    if(oldvalue[app->data.channelValue] != app->data.analogValue)
        oldvalue[app->data.channelValue] = app->data.analogValue;
    else
        return 1; // del this mesg


    if(UpConfig == 0){
        twInfoTable * content = uploadSetting(now);
        if(NULL == content){
            UpConfig = 0;
            return -1;
        }
        ret = configTableInit(content);
        if(ret == -1)
            UpConfig = 0;
        UpConfig = 1; //just upload once
    }

    ds = twDataShape_Create(twDataShapeEntry_Create("ChannelValue", NULL, TW_INTEGER));
    if (!ds) 
    {
        TW_LOG(TW_ERROR, "Error Creating datashape.");
        return -1;
    }
    twDataShape_AddEntry(ds, twDataShapeEntry_Create("AnalogValue", NULL, TW_NUMBER));

    twDataShape_AddEntry(ds, twDataShapeEntry_Create("CollectTime", NULL, TW_DATETIME));

    content = twInfoTable_Create(ds);
    if (!content) 
    {
        TW_LOG(TW_ERROR, "Watersystem - Error creating infotable");
        twDataShape_Delete(ds); 
        return -1;
    }


    printf("\n\n\nthe board time now is%llu\n\n\n\n", now);
    /* populate the InfoTableRow with arbitrary data */

    TW_LOG(TW_DEBUG, "channel %d, analogValue %lf", app->data.channelValue, app->data.analogValue);
    row = twInfoTableRow_Create(twPrimitive_CreateFromInteger(app->data.channelValue + 1));
    if (!row) 
    {
        TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable row");
        twInfoTable_Delete(content);	
        return -1;
    }

    twInfoTableRow_AddEntry(row, twPrimitive_CreateFromNumber(app->data.analogValue));  //模拟量值
    twInfoTableRow_AddEntry(row, twPrimitive_CreateFromDatetime(now));        //time
    /* add the InfoTableRow to the InfoTable */
    twInfoTable_AddRow(content, row);
    code = twApi_InvokeService(TW_THING, AdaptorID, "UploadCollectCardChannelAnalog", content, &it, -1, TRUE); //success return 0  调用云平台函数
    if (code) 
    {
        TW_LOG(TW_ERROR, "Error invoking service on Platform. EntityName: %s ServiceName: %s", AdaptorID, "UploadCollectCardChannelAnalog");
    }     
    return 1;
}


twInfoTable* tw_CreateUARTInfoTable(DATETIME now, int Num, tw_Public *ptr)
{
    /* Make the request to the server */

    return NULL;
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
		if(!fwrite(&p->data , sizeof(waterData) , 1, fp))
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

u16 usMBCRC16(u8 *srcMsg, u16 dataLen)
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
