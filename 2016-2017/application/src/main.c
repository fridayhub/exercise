/*
 *  Copyright (C) 2015 ThingWorx Inc.
 *
 */
#include "include.h"
#include "service.h"
//save mac address
char mac[13] = "";  

char now_time[25];

const char *configFilePath = "./NAME.conf";
const char *FIFO_PATH = "/data/swap";
int fd232 = -1;
int fd485 = -1;
int ledfd = -1;
int contfd = -1;
tw_Public tw_SystemInfo;
TW_MUTEX RS232recvRxMutex;
TW_MUTEX RS485recvRxMutex;

void *handle232=NULL;
void *handle485=NULL;
int readAdaptorID(char *adaptorID, char *systype);
int readConfigOptions(char *FileName);


/* Declarations for our threads */
#define NUM_WORKERS 5
twList * workerThreads = NULL;
twThread * apiThread = NULL;
twThread * RS232CollectionThread = NULL;
twThread * RS485CollectionThread = NULL;
twThread * fileWatcherThread = NULL;
twThread * serverFuncThread = NULL;
twThread * ledBlinkThread = NULL;

char  g_ConnectServer=FALSE;
volatile char  g_SyncServer = TRUE;
char g_SyncCnt=0;
char g_CheckConnect=FALSE;
//根据NAME.conf的配置信息修改
/****************************************************/
char softVersion[50];
char hardVersion[50];
char TW_HOST[50];
int16_t port; 
char TP_HOST[50];
int16_t tp_port; 
//get ip and port from test board
volatile unsigned char TS_HOST[100];        //ip地址
volatile unsigned short  ts_port = 0;      //端口
volatile unsigned short  send485_flag = 0;  
int alarm_state = 0;
volatile unsigned short  send485_flag2 = 0; 



int controllerID;
int controllerType;
int16_t uartselect;// 1 is 232 , 2 is 485 , 3 is all
int  baudrate232;
int  baudrate485;
char dynamicName232[50];
char dynamicName485[50];
char systype[10];
static int PortSquence=0; //上传次序  防止大流量无法交替上传
/****************************************************/
struct stat info; //save update file state
#define TW_APP_KEY "b26144d2-b9d2-4e4f-98c5-6e6850314afb"
#define NO_TLS

#define UPDATE_SHELL_RATE_MSEC (5*60*1000 * 1000)

char Iccid[21] = "12345678900987654321";

void getIccid()
{
    int fdi, n = 0;
    fdi = open("/etc/iccid", O_RDONLY);
    if(fdi < 0){
        TW_LOG(TW_ERROR, "Can't open iccid file");
    }
    memset(Iccid, 0, sizeof(Iccid));
    printf("Leng is %d:\n", sizeof(Iccid));
    n = read(fdi, Iccid, 20);
    if(n < 20){
        TW_LOG(TW_ERROR, "Read iccid faild");
        strncpy (Iccid, "12345678901234567890", 20);
    }
    printf("Read ICCID: %s\n", Iccid);
    close(fdi);
}


/*get updata tar from server*/
void getUpdateFileTask(DATETIME now, void * params) {
    /* Make the request to the server */
    twInfoTable * it = NULL;
    twInfoTable * transferInfo = NULL;
    int code;

    if (g_ConnectServer == TRUE){
        //code = twApi_InvokeService(TW_SUBSYSTEM, "getNewFileFromThingworx", "Copy", it, &transferInfo, 60, FALSE);
        code = twApi_InvokeService(TW_THING, AdaptorID, "getNewFileFromThingworx", transferInfo, &it, -1, TRUE);
        if (code) {
            TW_LOG(TW_ERROR, "Error invoking service on Platform. EntityName: %s ServiceName: %s", AdaptorID, "getNewFileFromThingworx");
            if (transferInfo) twInfoTable_Delete(transferInfo);
        }
		twInfoTable_Delete(transferInfo); 
    }

}



void shutdownTask(DATETIME now, void * params) 
{
	TW_LOG(TW_FORCE,"shutdownTask - Shutdown service called.  SYSTEM IS SHUTTING DOWN");
    g_ConnectServer=FALSE;
	twApi_UnbindThing(AdaptorID);
	twSleepMsec(100);
	twApi_Delete();
	twLogger_Delete();
	exit(0);	
}

/*******************
 * run update shell task
 *******************/
void runUpdateShellTask(DATETIME now, void * params) {
	twInfoTable * content = NULL;
	twInfoTable * it = NULL;
	int code= 0; 
	twDataShape * ds = 0;
    twInfoTableRow *row = 0;
	char status;
	if(FALSE == g_CheckConnect)
	{
		g_CheckConnect=TRUE;
		return;
	}
	ds = twDataShape_Create(twDataShapeEntry_Create("adaptorName", NULL, TW_STRING));
    if (!ds) 
	{
		TW_LOG(TW_ERROR, "Error Creating datashape.");
		return NULL;
	}
	
	content= twInfoTable_Create(ds);
    if (!content) 
	{
		TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable");
		twDataShape_Delete(ds); 
		return NULL;
	}
	
    row = twInfoTableRow_Create(twPrimitive_CreateFromString(AdaptorID, TRUE));
	if (!row) {
		TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable row");
		twInfoTable_Delete(content);	
		return NULL;
	}

	twInfoTable_AddRow(content, row);
	code = twApi_InvokeService(TW_THING, "PublicServicesThing", "getAdaptorIsConnected", content, &it, -1, FALSE); //success return 0  调用云平台函数
    if(!code)
    {
		twInfoTable_GetBoolean(it, "result", 0, &status);
		if(status == TRUE)
	        TW_LOG(TW_DEBUG, "Public Function Check Connect is OK");
		else
		{			
			if(FALSE == g_SyncServer) //如果有数据要上载，此处无需重连
			{
				twInfoTable_Delete(content);
				return;
			}
			int err = 0;
			err = twApi_Disconnect("Duty cycle off time");
			err = twApi_Connect(CONNECT_TIMEOUT, twcfg.connect_retries);//防止网络通畅无法上传
			if(!err)
			{
				TW_LOG(TW_DEBUG, "Public Function Check ReConnect is Succeed");
				twInfoTable_Delete(content);
				return;
			}
		}
    }
    else
    {
		
		TW_LOG(TW_DEBUG, "Public Server Repect Err");
        if(FALSE == g_SyncServer) //如果有数据要上载，此处无需重连
            return;
      	int err = 0;
        err = twApi_Disconnect("Duty cycle off time");
        err = twApi_Connect(CONNECT_TIMEOUT, twcfg.connect_retries);//防止网络通畅无法上传
        if(!err)
        {
            TW_LOG(TW_DEBUG, "Public Function Check ReConnect is Succeed");
            return;
        }
        TW_LOG(TW_DEBUG, "Public Function Check Connect is ERR");
    }
	twInfoTable_Delete(content);
}


/***************
Data Collection Task
****************/
/*
This function gets called at the rate defined in the task creation.  The SDK has 
a simple cooperative multitasker, so the function cannot infinitely loop.
Use of a task like this is optional and not required in a multithreaded
environment where this functonality could be provided in a separate thread.
*/
#define DATA_COLLECTION_RATE_MSEC (5 * 1000) // SDK 最终以微秒usleep 计算， 删除SDK 中乘1000，所以需要修改此时间
#define DATA_COMMIT_RATE_MSEC  (10 * 1000)

static int SystemInit(void)
{
	twDataShape * ds = 0;
	int err = 0;
	
	twLogger_SetLevel(TW_TRACE);
	twLogger_SetIsVerbose(1);
	TW_LOG(TW_FORCE, "Startiing to connect");
	err = twApi_Initialize(TW_HOST, port, TW_URI, TW_APP_KEY, NULL, MESSAGE_CHUNK_SIZE, MESSAGE_CHUNK_SIZE, TRUE);
    printf("Initialize return is :%d\n", err);
	if (err) 
	{
		TW_LOG(TW_ERROR, "Error initializing the API");
		return 0; 
	} 
	else 
	{
		TW_LOG(TW_FORCE, "twApi_Initialize OK");
	}
	/* Allow self signed certs */
	twApi_SetSelfSignedOk();
	twApi_DisableEncryption();
    return 1;
}
static int ReConnect_Server(void)
{
	
	int err = 0;
	err = twApi_Disconnect("Duty cycle off time");
    err = twApi_Connect(CONNECT_TIMEOUT, twcfg.connect_retries);//防止网络通畅无法上传
    if(!err)
    {
		g_SyncServer=TRUE;
		return 0;
    }
	TW_LOG(TW_ERROR,"main: Server connection failed after %d attempts.	Error Code: %d", twcfg.connect_retries, err);
	g_SyncServer=FALSE;
	return -1; //先返回,等待下次进入在传输数据
}

static twInfoTable* GetCurrentFrame(DATETIME now)
{
	
    twInfoTable * content = NULL;

    content = CreateNODEInfoTable(now , 0 , &tw_SystemInfo, contfd);
    if( content)
    {
        return content;
    }

    if(PORT_RS232 == uartselect)
    {
        twMutex_Lock(RS232recvRxMutex);
        content=CreateUARTInfoTable232(now , 0 , &tw_SystemInfo);
        twMutex_Unlock(RS232recvRxMutex);
    }
	else if(PORT_RS485 == uartselect)
	{
		twMutex_Lock(RS485recvRxMutex);
		content=CreateUARTInfoTable485(now , 0 , &tw_SystemInfo);
		twMutex_Unlock(RS485recvRxMutex);
	}
	else
	{
		if(0 == PortSquence)
		{
			PortSquence=1;
			twMutex_Lock(RS232recvRxMutex);
			content=CreateUARTInfoTable232(now , 0 , &tw_SystemInfo);
			twMutex_Unlock(RS232recvRxMutex);
			if(NULL == content)
			{	
				PortSquence=0;			
				twMutex_Lock(RS485recvRxMutex);
				content=CreateUARTInfoTable485(now , 0 , &tw_SystemInfo);
				twMutex_Unlock(RS485recvRxMutex);
			}
		}
		else
		{
			PortSquence=0;
			twMutex_Lock(RS485recvRxMutex);
			content=CreateUARTInfoTable485(now , 0 , &tw_SystemInfo);
			twMutex_Unlock(RS485recvRxMutex);
			if(NULL == content)
			{	
				PortSquence=1;			
				twMutex_Lock(RS232recvRxMutex);
				content=CreateUARTInfoTable232(now , 0 , &tw_SystemInfo);
				twMutex_Unlock(RS232recvRxMutex);
			}
		}
	}
	return content;
}
static void DeleteCurrentFrame(void)
{	
	if(PORT_RS232 == uartselect)
	{		
		twMutex_Lock(RS232recvRxMutex);
		Delete_CurrentNode232();
		twMutex_Unlock(RS232recvRxMutex);
	}
	else if(PORT_RS485 == uartselect)
	{
		twMutex_Lock(RS485recvRxMutex);
		Delete_CurrentNode485();
		twMutex_Unlock(RS485recvRxMutex);
	}
	else
	{
		if(1 == PortSquence)
		{	twMutex_Lock(RS232recvRxMutex);
			Delete_CurrentNode232();
			twMutex_Unlock(RS232recvRxMutex);
		}
		else
		{
			twMutex_Lock(RS485recvRxMutex);
			Delete_CurrentNode485();
			twMutex_Unlock(RS485recvRxMutex);
		}
	}
}

static void SaveList(void)
{
	
	if(PORT_RS232 == uartselect)
	{		
		twMutex_Lock(RS232recvRxMutex);
		SaveList_AsFile232();
		twMutex_Unlock(RS232recvRxMutex);
	}
	else if(PORT_RS485 == uartselect)
	{
		twMutex_Lock(RS485recvRxMutex);
		SaveList_AsFile485();
		twMutex_Unlock(RS485recvRxMutex);
	}
	else
	{
		twMutex_Lock(RS232recvRxMutex);
		SaveList_AsFile232();
		twMutex_Unlock(RS232recvRxMutex);
		twMutex_Lock(RS485recvRxMutex);
		SaveList_AsFile485();
		twMutex_Unlock(RS485recvRxMutex);
	}
}


void handler_ServerFunction(DATETIME now, void * params)
{
	/* Make the request to the server */
	twInfoTable * content = NULL;
	twInfoTable * it = NULL;
	int code= 0; 
	#if 0
	if(FALSE == g_ConnectServer)		//等待连接成功
		return;
	#endif
	if(FALSE == g_SyncServer)
	{
		if(0 == Is_ConnectServer("www.baidu.com"))
		{
				g_SyncServer=TRUE;
                if(-1 == ReConnect_Server())
                {
                    g_SyncServer=FALSE;
                    return;
                }
		}
		else
			return;
		
		SaveList();
			
	}
	now = twGetSystemTime(TRUE);
	content = GetCurrentFrame(now);
	if(NULL == content)
	{
		return;	
	}
	
	printf("ServerType=%d\n",tw_SystemInfo.ServerType);
	sendPropertyUpdate();
	switch(tw_SystemInfo.ServerType)
	{
		case SIEMENS_JB_TGZL_FC18R:
			code = twApi_InvokeService(TW_THING, AdaptorID, "setDeviceState2DBByxmz", content, &it, -1, TRUE); //success return 0  调用云平台函数
			if (code) 
			{
				TW_LOG(TW_ERROR, "Error invoking service on Platform. EntityName: %s ServiceName: %s", AdaptorID, "setDeviceState2DBByxmz");
			} 
			break;
        case SET_CONTROLLER_STATE: //485 用到 1 
		case SET_DEVICE_STATE:		// 2
			code = twApi_InvokeService(TW_THING, AdaptorID, "setDeviceState", content, &it, -1, TRUE); //success return 0  调用云平台函数
			if (code) 
			{				
				TW_LOG(TW_ERROR, "Error invoking service on Platform. EntityName: %s ServiceName: %s", AdaptorID, "setDeviceState");
				TW_LOG(TW_ERROR , "CODE=%d",code); 
			}
			break;
		case SET_OPERATOR_INFO:		// 4
			code = twApi_InvokeService(TW_THING, AdaptorID, "setOperatorInfomation", content, &it, -1, FALSE); //success return 0  调用云平台函数				
			if (code) 
			{
				TW_LOG(TW_ERROR, "Error invoking service on Platform. EntityName: %s ServiceName: %s", AdaptorID, "setOperatorInfomation");
				TW_LOG(TW_ERROR , "CODE=%d  off=%d",code,OFFLINE_MSG_STORE);
			}
			break;
		case READ_REMOTE_LEVEL_STATUS:
			code = twApi_InvokeService(TW_THING, AdaptorID, "SetImpowerToControllerStatus", content, &it, -1, TRUE); //success return 0  调用云平台函数
			if (code) 
			{
				TW_LOG(TW_ERROR, "Error invoking service on Platform. EntityName: %s ServiceName: %s", AdaptorID, "SetImpowerToControllerStatus");
			}
			break;
		case READ_CONTROLLER_ID:
			code = twApi_InvokeService(TW_THING, AdaptorID, "SetRemoteThingUDID", content, &it, -1, TRUE); //success return 0  调用云平台函数
			if (code) 
			{
				TW_LOG(TW_ERROR, "Error invoking service on Platform. EntityName: %s ServiceName: %s", AdaptorID, "SetRemoteThingUDID");
			}
			break;
		case READ_REMOTE_STATUS:
			code = twApi_InvokeService(TW_THING, AdaptorID, "SetRemoteThingOperatorStatus", content, &it, -1, TRUE); //success return 0  调用云平台函数
			if (code) 
			{
				TW_LOG(TW_ERROR, "Error invoking service on Platform. EntityName: %s ServiceName: %s", AdaptorID, "SetRemoteThingOperatorStatus");
			}     
			break;
		default:
			return;
	}
	
	twInfoTable_Delete(content);
	if(!code)		//succeed sync date to server
	{
		DeleteCurrentFrame();
		g_SyncServer = TRUE;
		g_SyncCnt=0;
		TW_LOG(TW_TRACE, "*****************Sync Data*****************");
	}
	else			
	{
		g_SyncServer = FALSE;
		if(code != TW_GATEWAY_TIMEOUT)
			g_SyncCnt++;
		if(g_SyncCnt>4)
		{
			g_SyncCnt=0;
			TW_LOG(TW_TRACE, "Sync Count err");
			DeleteCurrentFrame(); //当前节点上载失败超过5次
		}
		
		TW_LOG(TW_TRACE, "_________________Sync Faild___________________%d",code);
	}
	
}



void RS232dataCollectionTask(DATETIME now, void * params) 
{
    /* Make the request to the server */
	twMutex_Lock(RS232recvRxMutex);
	GetUARTInformation232(fd232, PORT_RS232);  //RS232	
    twMutex_Unlock(RS232recvRxMutex);
}
void RS485dataCollectionTask(DATETIME now, void * params) 
{
    /* Make the request to the server */
	twMutex_Lock(RS485recvRxMutex);
	GetUARTInformation485(fd485, PORT_RS485);  //RS485	
    twMutex_Unlock(RS485recvRxMutex);
}

/*****************
  Service Callbacks 
 ******************/

/* Example of handling multiple services in a callback */
enum msgCodeEnum multiServiceHandler(const char * entityName, const char * serviceName, twInfoTable * params, twInfoTable ** content, void * userdata) 
{
    char *softWareVersion="111";
    TW_LOG(TW_TRACE, "multiServiceHandler - Function called");
    if (!content) 
	{
        TW_LOG(TW_ERROR, "multiServiceHandler - NULL content pointer");
        return TWX_BAD_REQUEST;
    }
    if (strcmp(entityName, AdaptorID) == 0) 
	{
        if (strcmp(serviceName, "getSystemState") == 0) 
		{//test
            DATETIME now = twGetSystemTime(TRUE); //update time
            //if ( !(GetUartInformation(&thingData)))  //update thingData
            //   thingData.dataFlag = 1;
            //*content = CreateUARTInfoTable(now , 0 , &tw_SystemInfo );//test
            if (*content) 
                return TWX_SUCCESS;
            return TWX_ENTITY_TOO_LARGE;
        } 
		else if (strcmp(serviceName, "SystemRestart") == 0) 
		{
            /* Create a task to handle the shutdown so we can respond gracefully */
            //twApi_CreateTask(1, shutdownTask);
            system("reboot");
        } 
		else if (strcmp(serviceName, "AdaptorCreateSuccess") == 0) 
		{//is alive,now can call service           
			g_ConnectServer=TRUE;
			TW_LOG(TW_DEBUG, "------------------Connect Server Succeed-----------");
            return TWX_SUCCESS;
        }
		else if(strcmp(serviceName, "needUpdateFile") ==0)
		{
            //run get file server
            printf("Now get update file from server. \n");
            DATETIME now = twGetSystemTime(TRUE);
            getUpdateFileTask(now, NULL);
        } 	
		else         
            return TWX_NOT_FOUND;	
    }
    return TWX_NOT_FOUND;	
}


/*绑定动态库函数 -1 is err , 2 is warring/remote function  0 is succeed*/
static int bound_232dll_fouction(const char *handle)
{
	*(void **)&InitList232=dlsym(handle , "tw_InitList");
	if(InitList232 == NULL)
	{
		printf("InitList dlsym failed\n");
		return -1;
	}
    else
		printf("InitList dlsym succeed\n");
	if(-1==InitList232(fd232))
	{
		printf("Malloc for list faild\r\n");
		return -1;
	}
	*(void **)&DestroyList232=dlsym(handle , "tw_DestroyList");
	if(DestroyList232 == NULL)
	{
		printf("DestroyList dlsym failed\n");
		return -1;
	}
    else
		printf("DestroyList dlsym succeed\n");
	*(void **)&Delete_CurrentNode232=dlsym(handle , "tw_Delete_CurrentNode");
	if(Delete_CurrentNode232 == NULL)
	{
		printf("Delete_CurrentNode dlsym failed\n");
		return -1;
	}
    else
		printf("Delete_CurrentNode dlsym succeed\n");
	/*binding two function(greate server & get uart news) for main task use*/
	*(void **)(&CreateUARTInfoTable232)=dlsym(handle , "tw_CreateUARTInfoTable");
	if(CreateUARTInfoTable232 == NULL)
	{
		printf("CreateUARTInfoTable dlsym failed\n");
		return -1;
	}
	else
		printf("CreateUARTInfoTable dlsym succeed\n");
	*(void **)&GetUARTInformation232 = dlsym(handle,"tw_GetUARTInformation");
	if(GetUARTInformation232 == NULL)
	{
		printf("GetUARTInformation dlsym failed\n");
		return -1;
	}
	else 
		printf("GetUARTInformation dlsym success\n");
	*(void **)&SaveList_AsFile232 = dlsym(handle,"tw_SaveList_AsFile");
	if(SaveList_AsFile232 == NULL)
	{
		printf("SaveList_AsFile232 dlsym failed\n");
		return -1;
	}
	else 
		printf("SaveList_AsFile232 dlsym success\n");
	return 0;
}


/*绑定动态库函数 -1 is err , 2 is warring/remote function  0 is succeed*/
static int bound_485dll_fouction(const char *handle)
{
	*(void **)&InitList485=dlsym(handle , "tw_InitList");
	if(InitList485 == NULL)
	{
		printf("InitList dlsym failed\n");
		return -1;
	}
    else
		printf("InitList dlsym succeed\n");
	if(-1==InitList485(fd485))
	{
		printf("Malloc for list faild\r\n");
		return -1;
	}
	*(void **)&DestroyList485=dlsym(handle , "tw_DestroyList");
	if(DestroyList485 == NULL)
	{
		printf("DestroyList dlsym failed\n");
		return -1;
	}
    else
		printf("DestroyList dlsym succeed\n");
	*(void **)&Delete_CurrentNode485=dlsym(handle , "tw_Delete_CurrentNode");
	if(Delete_CurrentNode485 == NULL)
	{
		printf("Delete_CurrentNode dlsym failed\n");
		return -1;
	}
    else
		printf("Delete_CurrentNode dlsym succeed\n");
	*(void **)(&CreateUARTInfoTable485)=dlsym(handle , "tw_CreateUARTInfoTable");
	if(CreateUARTInfoTable485 == NULL)
	{
		printf("CreateUARTInfoTable dlsym failed\n");
		return -1;
	}
	else
		printf("CreateUARTInfoTable dlsym succeed\n");
	*(void **)&GetUARTInformation485 = dlsym(handle,"tw_GetUARTInformation");
	if(GetUARTInformation485 == NULL)
	{
		printf("GetUARTInformation dlsym failed\n");
		return -1;
	}
	else 
		printf("GetUARTInformation dlsym success\n");
	*(void **)&SaveList_AsFile485 = dlsym(handle,"tw_SaveList_AsFile");
	if(SaveList_AsFile485== NULL)
	{
		printf("SaveList_AsFile485 dlsym failed\n");
		return -1;
	}
	else 
		printf("SaveList_AsFile dlsym success\n");
	*(void **)&readRemoteController485 = dlsym(handle,"tw_readRemoteController");
	if(readRemoteController485 == NULL)
	{
		printf("readRemoteControllerAuthorizationStatus dlsym failed\n");
		return -2;  //don't exit because else .so hasn't remote function
	}
	else
	      printf("readRemoteControllerAuthorizationStatus dlsym succeed\n");

	*(void **)&setRemoteController485 = dlsym(handle,"tw_setRemoteController");
	if(setRemoteController485 == NULL)
	{
		printf("setRemoteControllerAuthorizationStatus dlsym failed\n");
		return -2;
	}
	else
	      printf("setRemoteControllerAuthorizationStatus dlsym succeed\n");
	*(void **)&RemoteControl485 = dlsym(handle , "tw_RemoteControl");
	if(RemoteControl485 == NULL)
	{
		printf("RemoteControl dlsym failed\n");
		return -2;
	}
	else
	      printf("RemoteControl dlsym succeed\n");
	*(void **)&setRemoteControlStatus = dlsym(handle , "tw_setRemoteControlStatus");
	if(setRemoteControlStatus == NULL)
	{
		printf("setRemoteControlStatus dlsym failed\n");
		return -2;
	}
	else
	      printf("setRemoteControlStatus dlsym succeed\n");
	return 0;
}
//根据配置文件打开或者关闭串口和动态库
static int set_config(void)
{
	int ret=0;

    //led for uart data get
    ledfd = open("/sys/class/leds/led1/brightness", O_RDWR | O_NOCTTY | O_NDELAY);
    if(ledfd < 0){
        perror("open brightness");
        exit(-1);
    }

   
	if(PORT_RS232==uartselect)
	{
		fd232= open232Port("/dev/ttymxc3", baudrate232);
		if(-1==fd232)
	    {
			printf("Open UART232 Faild\r\n");
			printf("#########fd==%d\n",fd232);
			return -1;
	    }
    	handle232= dlopen(dynamicName232,RTLD_LAZY);
		if(handle232 == NULL)
		{
			printf("processRecvData232 dlopen failed\n");
		    return -1;
	    }
	    else
		   printf("processRecvData232 dlopen sssssed\n");
		return bound_232dll_fouction(handle232);
	}
   	else if(PORT_RS485==uartselect)
   	{
		fd485= open485Port("/dev/ttymxc1", baudrate485);
		if(-1==fd485)
	    {
			printf("Open UART485 Faild\r\n");
			printf("#########fd==%d\n",fd485);
			return -1;
	    }
    	handle485= dlopen(dynamicName485,RTLD_LAZY);
		if(handle485 == NULL)
		{
			printf("processRecvData485 dlopen failed\n");
		    return;
	    }
	    else
		   printf("processRecvData485 dlopen sssssed\n");
		return bound_485dll_fouction(handle485);
	}
	else
	{
		fd232= open232Port("/dev/ttymxc3", baudrate232);
		if(-1==fd232)
	    {
			printf("Open UART232 Faild\r\n");
			printf("#########fd==%d\n",fd232);
			return -1;
	    }
		fd485= open485Port("/dev/ttymxc1", baudrate485);
		if(-1==fd485)
	    {
			printf("Open UART485 Faild\r\n");
			printf("#########fd==%d\n",fd485);
			return -1;
	    }
		handle232= dlopen(dynamicName232,RTLD_LAZY);
		if(handle232 == NULL)
		{
			printf("processRecvData232 dlopen failed\n");
		    return;
	    }
	    else
		   printf("processRecvData232 dlopen sssssed\n");
		ret = bound_232dll_fouction(handle232);
		if(-1 == ret)
		{
			return -1;
		}
		handle485= dlopen(dynamicName485,RTLD_LAZY);
		if(handle485 == NULL)
		{
			printf("processRecvData485 dlopen failed\n");
		    return;
	    }
	    else
		   printf("processRecvData485 dlopen sssssed\n");
		return bound_485dll_fouction(handle485);
	}
}

void systemexit(void)
{
    /* 
       twApi_Delete also cleans up all singletons including 
       twFileManager, twTunnelManager and twLogger. 
       */
    twList_Delete(workerThreads);
    twThread_Delete(apiThread);
    twThread_Delete(fileWatcherThread);
    twThread_Delete(serverFuncThread);
    twThread_Delete(ledBlinkThread);

    close(ledfd);
    close(contfd);

    if(PORT_RS232==uartselect)
    {
        twThread_Delete(RS232CollectionThread);
        dlclose(handle232); 
        DestroyList232();
        close232Port(fd232);
    }
    else if(PORT_RS485 == uartselect)
    {
        twThread_Delete(RS485CollectionThread);
        dlclose(handle485); 
        DestroyList485();
        close485Port(fd485);
    }
    else
    {
        twThread_Delete(RS232CollectionThread);
        twThread_Delete(RS485CollectionThread);
        dlclose(handle232); 
        DestroyList232();
        close232Port(fd232);
        dlclose(handle485); 
        DestroyList485();
        close485Port(fd485);
    }

    /* Shutdown */
    twApi_UnbindThing(AdaptorID);
    twSleepMsec(100);

    twApi_Delete(); 
    exit(0);
}


/******初始化服务*********/
int ServerInit(void)
{
	twDataShape * ds = 0;
	int err = 0;
    int ret;

    ret = SystemInit();
    if(!ret)
        return 0;
    /* Set up for connecting through an HTTP proxy: Auth modes supported - Basic, Digest and None */
    /****
      twApi_SetProxyInfo("10.128.0.90", 3128, "proxyuser123", "thingworx");
     ****/ 
    //register remote controller
    if(PORT_RS232!=uartselect)
        remoteContorller();
    /* API now owns that datashape pointer, so we can reuse it */
    ds = NULL;
    /* Register our services that have inputs */
    /* Create DataShape */
    ds = twDataShape_Create(twDataShapeEntry_Create("AdaptorID", NULL, TW_STRING));
    if (!ds) 
    {
        TW_LOG(TW_ERROR, "Error Creating datashape for getDeviceState");
        return 0;
        //exit(1);
    }
    twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerType", NULL, TW_STRING));
    twDataShape_AddEntry(ds, twDataShapeEntry_Create("CurrentTime", NULL, TW_STRING));
    twDataShape_AddEntry(ds, twDataShapeEntry_Create("SystemConfigState", NULL, TW_STRING));
    //add getSystemState server
    twApi_RegisterService(TW_THING, AdaptorID, "getSystemState", NULL, ds, TW_STRING, NULL, multiServiceHandler, NULL);
    /* Register our services that don't have inputs */
    twApi_RegisterService(TW_THING, AdaptorID, "AdaptorCreateSuccess", NULL, NULL, TW_NOTHING, NULL, multiServiceHandler, NULL);
    twApi_RegisterService(TW_THING, AdaptorID, "SystemRestart", NULL, NULL, TW_NOTHING, NULL, multiServiceHandler, NULL);
    /*Register our service that get software newest version from BJ server*/
    twApi_RegisterService(TW_THING, AdaptorID, "needUpdateFile", NULL, NULL, TW_NOTHING, NULL, multiServiceHandler, NULL);		
    //regist get time service
    ds = NULL;
    ds = twDataShape_Create(twDataShapeEntry_Create("AdaptorID", NULL, TW_DATETIME));
    if (!ds) 
    {
        TW_LOG(TW_ERROR, "Error Creating datashape for get time");
        return 0;
        //exit(1);
    }
    //get time from ThingWorx platform
    twApi_RegisterService(TW_THING, AdaptorID, "timeSynchronization", NULL, ds, TW_DATETIME, NULL, getPlatformtime, NULL);
    /* Register our Events */
    /* Create DataShape */
    ds = twDataShape_Create(twDataShapeEntry_Create("message",NULL,TW_STRING));
    if (!ds) 
    {
        TW_LOG(TW_ERROR, "Error Creating datashape.");
        return 0;
        //exit(1);
    } 
    else 
    {
        TW_LOG(TW_FORCE, "twDataShape_Create Message OK");
    }
    /* Event datashapes require a name */
    twDataShape_SetName(ds, "SteamSensor.Fault");
    /* Register the service */
    twApi_RegisterEvent(TW_THING, AdaptorID, "AlarmFault", "Steam sensor event", ds);
    ds = NULL;	
    twApi_RegisterProperty(TW_THING, AdaptorID, "ID", TW_NUMBER, NULL, "ALWAYS", 0, propertyHandler, NULL);
    twApi_RegisterProperty(TW_THING, AdaptorID, "protocolType", TW_INTEGER, NULL, "ALWAYS", 0, propertyHandler, NULL); //增加协议类型属性232 or 485
    twApi_RegisterProperty(TW_THING, AdaptorID, "type", TW_STRING, NULL, "ALWAYS", 0, propertyHandler, NULL);
    twApi_RegisterProperty(TW_THING, AdaptorID, "HardwareVersion", TW_STRING, NULL, "ALWAYS", 0, propertyHandler, NULL);
    twApi_RegisterProperty(TW_THING, AdaptorID, "SoftwareVersion", TW_STRING, NULL, "ALWAYS", 0, propertyHandler, NULL);

    /* Register an authentication event handler */
    twApi_RegisterOnAuthenticatedCallback(AuthEventHandler, NULL); /* Callbacks only when we have connected & authenticated */

    /* Register a bind event handler */
    twApi_RegisterBindEventCallback(AdaptorID, BindEventHandler, NULL); /* Callbacks only when AdaptorID is bound/unbound */

    twApi_BindThing(AdaptorID);

    twFileManager_Create();

    /* Create our virtual directories */
    twFileManager_AddVirtualDir(AdaptorID, "tw", "/data/update");
    //twFileManager_AddVirtualDir(AdaptorID, "tw2", "/home/hakits/SOME");

    /* Register the file transfer callback function */
    twFileManager_RegisterFileCallback(fileCallbackFunc, NULL, FALSE, NULL);

    /* Connect to server */
    twApi_SetConnectTimeout(30000);
    err = twApi_Connect(CONNECT_TIMEOUT, twcfg.connect_retries);
    if (err) 
    {
        TW_LOG(TW_ERROR,"main: Server connection failed after %d attempts.  Error Code: %d", twcfg.connect_retries, err);
        return 0;
        //systemexit();
    } 

    TW_LOG(TW_DEBUG,"Wait for Server Repect");
    sendPropertyUpdate();
    sleep(15);
    sendPropertyUpdate();
    return 1;
}

/***********Create thread**************/
void CreateThread(void)
{
    /******************************************/
    /*           Create our threads           */
    /******************************************/
    /* Create and start our worker Threads */
    workerThreads = twList_Create(twThread_Delete);
    if (workerThreads) {
        int i = 0;
        for (i = 0; i < NUM_WORKERS; i++) {
            twThread * tmp = twThread_Create(twMessageHandler_msgHandlerTask, 5, NULL, TRUE);
            if (!tmp) {
                TW_LOG(TW_ERROR,"main: Error creating worker thread.");
                break;
            }
            twList_Add(workerThreads, tmp);
        }
    } else {
        TW_LOG(TW_ERROR,"main: Error creating worker thread list.");
    }

    /* Create and start a thread for the data collection function */
    if(PORT_RS232 == uartselect)		
    {
        RS232recvRxMutex = twMutex_Create();
        if(RS232recvRxMutex == NULL)
            twMutex_Delete(RS232recvRxMutex);
        RS232CollectionThread = twThread_Create(RS232dataCollectionTask, DATA_COLLECTION_RATE_MSEC, NULL, TRUE); 
        if( !RS232CollectionThread ){
            TW_LOG(TW_ERROR, "RS232CollectionThread create failed");
            exit(0);
        }
    }
    else if(PORT_RS485 == uartselect)
    {
        RS485recvRxMutex = twMutex_Create();
        if(RS485recvRxMutex == NULL)
            twMutex_Delete(RS485recvRxMutex);
        RS485CollectionThread = twThread_Create(RS485dataCollectionTask, DATA_COLLECTION_RATE_MSEC, NULL, TRUE); 
        if( !RS485CollectionThread ){
            TW_LOG(TW_ERROR, "RS485CollectionThread carete failed");
            exit(0);
        }
    }
    else
    {
        RS232recvRxMutex = twMutex_Create();
        if(RS232recvRxMutex == NULL)
            twMutex_Delete(RS232recvRxMutex);

        RS485recvRxMutex = twMutex_Create();
        if(RS485recvRxMutex == NULL)
            twMutex_Delete(RS485recvRxMutex);
        RS232CollectionThread = twThread_Create(RS232dataCollectionTask, DATA_COLLECTION_RATE_MSEC, NULL, TRUE); 
        RS485CollectionThread = twThread_Create(RS485dataCollectionTask, DATA_COLLECTION_RATE_MSEC, NULL, TRUE); 
        if(!RS232CollectionThread || !RS485CollectionThread){
            TW_LOG(TW_ERROR, "RS232CollectionThread RS485CollectionThread create error");
        }
    }
}


/***************
  Main Loop
 ****************/
/*
   Solely used to instantiate and configure the API.
   */
int main( int argc, char** argv ) 
{
#if defined NO_TLS
#else
#endif
	int err = 0;
#ifndef ENABLE_TASKER
	DATETIME nextDataCollectionTime = 0;
#endif
    int retrys; //reconncet to server
    int ret;

    //读取配置文件
    readConfigOptions(configFilePath);
    analysisOptions();
    
    if(TRUE != (readAdaptorID(AdaptorID, systype)))
	{
        TW_LOG(TW_ERROR, "Can't get AdaptorID\n"); 
        exit(1); 
    }
    printf("AdaptorID is : %s\n", AdaptorID);

    //get iccid
    getIccid();

	printf("dynamicName232=%s\r\n",dynamicName232);
	printf("dynamicName485=%s\r\n",dynamicName485);
	strcpy(tw_SystemInfo.dynamicName232 , dynamicName232);
	strcpy(tw_SystemInfo.dynamicName485 , dynamicName485);
	err = set_config();
	if(-1 == err)
	{
		return;
	}
	twLogger_SetLevel(TW_TRACE);
    twLogger_SetIsVerbose(1);
    TW_LOG(TW_FORCE, "Starting up");

    CreateThread(); //Create thread task
    if(strncmp(dynamicName232, "./NAME_232_test.so", sizeof("./NAME_232_test.so")) == 0 ){ //工装测试库
        printf("Board test function on!\n");
        while(1){
            if(ts_port < 65535 && ts_port > 80){
                sprintf(TW_HOST, "%d.%d.%d.%d", TS_HOST[0], TS_HOST[1], TS_HOST[2], TS_HOST[3]);
                printf("Get ip for test board is :%s\n", TW_HOST);
                port = ts_port;
               // if(ServerInit())
                    break;
            }
        }
    }else{ //正常通讯
        strcpy(TW_HOST, TP_HOST);
        port = tp_port; 
        if(!ServerInit()){
            systemexit();
         }
         
		/* Create and start a thread for the Api tasker function */
		 apiThread = twThread_Create(twApi_TaskerFunction, 5, NULL, TRUE);
		 /* Create and start a thread for the file watcher function */
		 fileWatcherThread = twThread_Create(runUpdateShellTask, UPDATE_SHELL_RATE_MSEC, NULL, TRUE); 
		 /* Create and start a thread for the tunnel manager function */
		 serverFuncThread = twThread_Create(handler_ServerFunction, DATA_COMMIT_RATE_MSEC, NULL, TRUE);
		 /* Create uart data led blink function*/
		 ledBlinkThread = twThread_Create(ledBlink, 1, (void *)&ledfd, TRUE);  //传递led描述符
		 
		 /* Check for any errors */
		 if (!apiThread || !fileWatcherThread || !serverFuncThread || !ledBlinkThread) {
			 TW_LOG(TW_ERROR,"main: Error creating a required thread.");
		 }
		 
		 /* Show that our threads are running */
		 TW_LOG(TW_DEBUG,"main: API Thread isRunning: %s", twThread_IsRunning(apiThread) ? "TRUE" : "FALSE");
		 //TW_LOG(TW_DEBUG,"main: Data Collection 232Thread isRunning: %s", twThread_IsRunning(RS232CollectionThread) ? "TRUE" : "FALSE");
		 //TW_LOG(TW_DEBUG,"main: Data Collection 485Thread isRunning: %s", twThread_IsRunning(RS485CollectionThread) ? "TRUE" : "FALSE");
		 TW_LOG(TW_DEBUG,"main: File Watcher Thread isRunning: %s", twThread_IsRunning(runUpdateShellTask) ? "TRUE" : "FALSE");
		 TW_LOG(TW_DEBUG,"main: Tunnel Manager Thread isRunning: %s", twThread_IsRunning(handler_ServerFunction) ? "TRUE" : "FALSE");
		 {
			 ListEntry * le = twList_Next(workerThreads, NULL);
			 int cnt = 0;
			 while (le && le->value) {
				 TW_LOG(TW_DEBUG,"main: Worker Thread [%d] isRunning: %s", cnt++, twThread_IsRunning((twThread *)le->value) ? "TRUE" : "FALSE");
				 le = twList_Next(workerThreads, le);
			 }
		 }

            
    }



	while(1) 
	{
		char in = 0;		
#ifndef ENABLE_TASKER
        printf("\n\nShould not be here\n\n");
		DATETIME now = twGetSystemTime(TRUE);
		twApi_TaskerFunction(now, NULL);
		twMessageHandler_msgHandlerTask(now, NULL);
		if (twTimeGreaterThan(now, nextDataCollectionTime)) 
		{
			if(PORT_RS232 == uartselect)		
				RS232dataCollectionTask(now, NULL);
			else if(PORT_RS485 == uartselect)
				RS485dataCollectionTask(now, NULL);
			else
			{
				RS232dataCollectionTask(now, NULL);
				RS485dataCollectionTask(now, NULL);
			}
			nextDataCollectionTime = twAddMilliseconds(now, DATA_COLLECTION_RATE_MSEC);
		}
#else
#if 0
        in = getch();
        if(in == 't'){
            printf("\n\n Start getThingworxTime\n\n");
            DATETIME now = twGetSystemTime(TRUE);
            getThingworxTime(now, NULL); 
        }else if (in == 'f'){
            printf("Now get update file from server. \n");
            DATETIME now = twGetSystemTime(TRUE);
            getUpdateFileTask(now, NULL);
        }else if (in == 'q') 
            break;
        else 
            ;
#endif
	GetContactStatus();

#endif
        twSleepMsec(5);
    }
systemexit();
}
