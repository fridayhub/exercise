#include "service.h"


/* Name of our thing */
//char *AdaptorID = "01_TX3252_00AEFAB87613";
char AdaptorID[100] = "";
extern char  g_ConnectServer;
extern char softVersion[50];
extern char hardVersion[50];
extern tw_Public tw_SystemInfo;
extern char TW_HOST[50];

extern char Iccid[20];

volatile int ledFlag;
/*led blink when date come from uart*/
void * ledBlink(DATETIME now, void *parm)
{
    int i = 0;
    int *ledfds = (int *)parm;

    while(ledFlag){
        while(i < 5){
            write(*ledfds, "1", 1); 
            usleep(40000);
            write(*ledfds, "0", 1); 
            usleep(40000);
            i++;
        }
    }   
}

/*****************
Helper Functions
*****************/
void sendPropertyUpdate(void) 
{
	/* Create the property list */
	/////////////////////////////////////////////////////	
//    getIccid();
    TW_LOG(TW_TRACE, "Get ICCID:%s", Iccid);
	propertyList * proplist = twApi_CreatePropertyList("ID", twPrimitive_CreateFromString(Iccid, TRUE), 1);
	if (!proplist) {
		TW_LOG(TW_ERROR,"sendPropertyUpdate: Error allocating property list");
		return;
	}
	twApi_AddPropertyToList(proplist, "type", twPrimitive_CreateFromString("TX3251", TRUE), 0);
	//twApi_AddPropertyToList(proplist, "belongProjectName", twPrimitive_CreateFromString("67859552X", TRUE), 0);
    twApi_AddPropertyToList(proplist, "protocolType", twPrimitive_CreateFromInteger(properties.protocolType), 0); //2  232通信方式, 1 485
    twApi_AddPropertyToList(proplist, "HardwareVersion", twPrimitive_CreateFromString(hardVersion, TRUE), 0); //硬件版本号
    twApi_AddPropertyToList(proplist, "SoftwareVersion", twPrimitive_CreateFromString(softVersion, TRUE), 0); //软件版本号
    printf("\n\nValue is %d\n", properties.protocolType);

	twApi_PushProperties(TW_THING, AdaptorID, proplist, -1, TRUE);
	twApi_DeletePropertyList(proplist);
}

/*****************
Property Handler Callbacks 
******************/
enum msgCodeEnum propertyHandler(const char * entityName, const char * propertyName,  twInfoTable ** value, char isWrite, void * userdata) {
	char * asterisk = "*";
	if (!propertyName) 
		propertyName = asterisk;
	TW_LOG(TW_TRACE,"propertyHandler - Function called for Entity %s, Property %s", entityName, propertyName);
	if (value) {
		if (isWrite && *value) {			
			if (strcmp(propertyName, "ID") == 0) 
				twInfoTable_GetNumber(*value, propertyName, 0, &properties.ID); 
			else if (strcmp(propertyName, "protocolType") == 0) 
				twInfoTable_GetInteger(*value, propertyName, 0, (char **)&properties.protocolType);
			else if (strcmp(propertyName, "belongProjectName") == 0) 
				twInfoTable_GetString(*value, propertyName, 0, (char **)&properties.belongProjectName);
			else return TWX_NOT_FOUND;
		}
		return TWX_SUCCESS;
	} else {
		TW_LOG(TW_ERROR,"propertyHandler - NULL pointer for value");
		return TWX_BAD_REQUEST;
	}
}

void getThingworxTime(DATETIME now, void *params)
{
    /* Make the request to the server */
	twInfoTable * content = NULL;
	twInfoTable * it = NULL;
	int code;

	if (g_ConnectServer == TRUE){		
		//content = CreateUARTInfoTable(now , 0 , &tw_SystemInfo);	//test
		if(content){
			code = twApi_InvokeService(TW_THING, AdaptorID, "timeSynchronization", content, &it, -1, TRUE); //success return 0  调用云平台函数
			if (code) {
				TW_LOG(TW_ERROR, "Error invoking service on Platform. EntityName: %s ServiceName: %s", AdaptorID, "getThingworxDateTime");
			}
			twInfoTable_Delete(content); 
        }
    }
	/* Update the properties on the server */
    sendPropertyUpdate();

}


/* Get platform time Callback*/
enum msgCodeEnum getPlatformtime(const char * entityName, const char * serviceName, twInfoTable * params, twInfoTable ** content, void * userdata) {
    TW_LOG(TW_TRACE, "getPlatformtime - Function called");
    if (!content) 
	{
        TW_LOG(TW_ERROR, "getPlatformtime - NULL content pointer");
        return TWX_BAD_REQUEST;
    }

    if (strcmp(entityName, AdaptorID) == 0) 
	{
        if(strcmp(serviceName, "timeSynchronization") == 0)
		{
            DATETIME current;
            TW_LOG(TW_FORCE, "NOW Process timeSynchronization");
            if (twInfoTable_GetDatetime(params, "CurrentTime", 0, &current))
			{
                TW_LOG(TW_ERROR, "timeSynchronization failed");
                return TWX_BAD_REQUEST;
            }
            printf("\n\n\nGet time %llu \n\n\n", current);
            return TWX_SUCCESS;
        }
        return TWX_NOT_FOUND;
    }
    return TWX_NOT_FOUND;
}


/*******************************************/
/*         Bind Event Callback             */
/*******************************************/
void BindEventHandler(char * entityName, char isBound, void * userdata) {
	if (isBound) TW_LOG(TW_FORCE,"BindEventHandler: Entity %s was Bound", entityName);
	else TW_LOG(TW_FORCE,"BindEventHandler: Entity %s was Unbound", entityName);
}


/*******************************************/
/*    OnAuthenticated Event Callback       */
/*******************************************/
void AuthEventHandler(char * credType, char * credValue, void * userdata) {
	if (!credType || !credValue) return;
	TW_LOG(TW_FORCE,"AuthEventHandler: Authenticated using %s = %s.  Userdata = 0x%x", credType, credValue, userdata);
	/* Could do a delayed bind here */
	/* twApi_BindThing(AdaptorID); */
}


/* remote controller Callback*/
enum msgCodeEnum remoteHandler(const char * entityName, const char * serviceName, twInfoTable * params, twInfoTable ** content, void * userdata) 
{
    TW_LOG(TW_TRACE, "remoteHandler - Function called %s",serviceName);
    if (!content) 
	{
        TW_LOG(TW_ERROR, "remoteHandler- NULL content pointer");
        return TWX_BAD_REQUEST;
    }

    int controllerid = 0, controllertype = 0, level = 0, controltype = 0, controlmarker = 0 , oparatorstatus;
    //unsigned char ccontrollerid[5] = {0}, ccontrollertype[5] = {0}, clevel[5] = {0}, ccontroltype[5] = {0}, ccontrolmarker[5] = {0};
    char *code;
    if (strcmp(entityName, AdaptorID) == 0) 
	{
        if (strcmp(serviceName, "ImpowerToController") == 0)
		{ //远程授权
            twInfoTable_GetInteger(params, "ControllerID", 0, &controllerid);
            twInfoTable_GetInteger(params, "ControllerType", 0, &controllertype);
            twInfoTable_GetInteger(params, "level", 0, &level);
            twInfoTable_GetString(params, "code", 0, &code);
            printf("\n\nImpowerToController  controllerid: %d, controllertype: %d, level: %d, code: %s\n\n", controllerid, controllertype, level, code);
            setRemoteController485(controllerid, controllertype, level, code); //设置控制器远程授权
            return TWX_SUCCESS;
        }
		else if (strcmp(serviceName, "ReadImpowerToControllerStatus") == 0) 
		{ //读取远程授权状态
            twInfoTable_GetInteger(params, "ControllerID", 0, &controllerid);
            twInfoTable_GetInteger(params, "ControllerType", 0, &controllertype);
            printf("\n\nReadImpowerToControllerStatus  controllerid: %d, controllertype: %d\n\n", controllerid, controllertype);
	    	readRemoteController485(controllerid,controllertype,0,NULL,72);//读取远程控制器授权状态
            return TWX_SUCCESS;
        }
		else if (strcmp(serviceName, "ReadRemoteThingUDID") == 0) 
		{ //读取远程控制器识别码
            twInfoTable_GetInteger(params, "ControllerID", 0, &controllerid);
            twInfoTable_GetInteger(params, "ControllerType", 0, &controllertype);
            printf("\n\nReadRemoteThingUDID  controllerid: %d, controllertype: %d\n\n", controllerid, controllertype);
	    	readRemoteController485(controllerid,controllertype,0,NULL,73);
            return TWX_SUCCESS;
        }
		else if ( strcmp(serviceName, "OperatorRemoteThing") == 0) 
		{ //远程控制
            twInfoTable_GetInteger(params, "ControllerID", 0, &controllerid);
            twInfoTable_GetInteger(params, "ControllerType", 0, &controllertype);
            twInfoTable_GetInteger(params, "ControlType", 0, &controltype);
            twInfoTable_GetInteger(params, "ControlMarker", 0, &controlmarker);
            printf("\n\nOperatorRemoteThing  controllerid: %d, controllertype: %d, ControlType: %d, ControlMarker: %d\n\n", controllerid, controllertype, controltype, controlmarker);
           	RemoteControl485(controllerid, controllertype, controltype, controlmarker);
			content =twInfoTable_CreateFromInteger("reslut",TRUE);
            return TWX_SUCCESS;
        }
		else if ( strcmp(serviceName, "ReadRemoteThingOperatorStatus") == 0) 
		{ //读远程控制状态
            twInfoTable_GetInteger(params, "ControllerID", 0, &controllerid);
            twInfoTable_GetInteger(params, "ControllerType", 0, &controllertype);
            printf("\n\nReadRemoteThingOperatorStatus  controllerid: %d, controllertype: %d\n\n", controllerid, controllertype);
	    	readRemoteController485(controllerid,controllertype,0,NULL,77);//读远程控制状态
            return TWX_SUCCESS;
        }
		else if ( strcmp(serviceName, "sendCanOperatorCommand") == 0) 
		{ //读远程控制状态
            twInfoTable_GetInteger(params, "ControllerID", 0, &controllerid);
            twInfoTable_GetInteger(params, "ControllerType", 0, &controllertype);
			twInfoTable_GetInteger(params, "oparatorStatus", 0, &oparatorstatus);
            printf("\n\nsendCanoperatorCommand  controllerid: %d, controllertype: %d oparatorstatus: %d\n\n", controllerid, controllertype,oparatorstatus);
	    	setRemoteControlStatus(controllerid,controllertype,oparatorstatus);//读远程控制状态
            return TWX_SUCCESS;
        }
        return TWX_NOT_FOUND;
    }
    return TWX_NOT_FOUND;
}

void remoteContorller(void)
{

	twDataShape * ds = 0;
    	/* API now owns that datashape pointer, so we can reuse it */    
    	//远程授权
	ds = NULL;
	/* Register our services that have inputs */
	/* Create DataShape */
	ds = twDataShape_Create(twDataShapeEntry_Create("AdaptorID", NULL, TW_STRING));
	if (!ds) 
	{
		TW_LOG(TW_ERROR, "Error Creating datashape for getDeviceState");
		exit(1);
	}
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerID", NULL, TW_INTEGER));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerType", NULL, TW_INTEGER));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("level", NULL, TW_INTEGER));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("code", NULL, TW_STRING));
    	//add getSystemState server  远程授权
	twApi_RegisterService(TW_THING, AdaptorID, "ImpowerToController", NULL, ds, TW_INTEGER, NULL, remoteHandler, NULL);

    	//读取远程授权状态  读取远程控制器识别码 读远程控制状态
    ds = NULL;
	ds = twDataShape_Create(twDataShapeEntry_Create("AdaptorID", NULL, TW_STRING));
	if (!ds) 
	{
		TW_LOG(TW_ERROR, "Error Creating datashape for getDeviceState");
		exit(1);
	}
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerID", NULL, TW_INTEGER));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerType", NULL, TW_INTEGER));
	
    twApi_RegisterService(TW_THING, AdaptorID, "ReadImpowerToControllerStatus", NULL, ds, TW_INTEGER, NULL, remoteHandler, NULL); //读取远程授权状态
	twApi_RegisterService(TW_THING, AdaptorID, "ReadRemoteThingUDID", NULL, ds, TW_INTEGER, NULL, remoteHandler, NULL); //读取远程控制器识别码
	twApi_RegisterService(TW_THING, AdaptorID, "ReadRemoteThingOperatorStatus", NULL, ds, TW_INTEGER, NULL, remoteHandler, NULL); //读远程控制状态
    twApi_RegisterService(TW_THING, AdaptorID, "sendCanOperatorCommand", NULL, ds, TW_INTEGER, NULL, remoteHandler, NULL); //读取远程授权状态
    //远程控制
    ds = NULL;
    ds = twDataShape_Create(twDataShapeEntry_Create("AdaptorID", NULL, TW_STRING));
    if (!ds) 
	{
		TW_LOG(TW_ERROR, "Error Creating datashape for getDeviceState");
		exit(1);
    }
    twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerID", NULL, TW_INTEGER));
    twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerType", NULL, TW_INTEGER));
    twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControlType", NULL, TW_INTEGER)); //控制类型
    twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControlMarker", NULL, TW_INTEGER)); //控制标志（指示）

    twApi_RegisterService(TW_THING, AdaptorID, "OperatorRemoteThing", NULL, ds, TW_INTEGER, NULL, remoteHandler, NULL); //读取远程授权状态


}


/*******************************************/                                                                                                 
/*    FileTransfer Event Callback          */                                                                                                 
/*******************************************/
void fileCallbackFunc (char fileRcvd, twFileTransferInfo * info, void * userdata) {                                                           
    char startTime[80];                                                                                                                       
    char endTime[80];                                                                                                                         
    if (!info) {                                                                                                                              
        TW_LOG(TW_ERROR,"fileCallbackFunc: Function called with NULL info");                                                                  
    }                                                                                                                                         
    twGetTimeString(info->startTime, startTime, "%Y-%m-%d %H:%M:%S", 80, 1, 1);                                                               
    twGetTimeString(info->endTime, endTime, "%Y-%m-%d %H:%M:%S", 80, 1, 1);                                                                   
    TW_LOG(TW_AUDIT,"\n\n*****************\nFILE TRANSFER NOTIFICATION:\nSource: %s:%s/%s\nDestination: %s:%s/%s\nSize: %9.0f\nStartTime: %s\nEndTime: %s\nDuration: %d msec\nUser: %s\nState: %s\nMessage: %s\nTransfer ID: %s\n*****************\n",
            info->sourceRepository, info->sourcePath, info->sourceFile, info->targetRepository, info->targetPath,                                 
            info->targetFile, info->size, startTime, endTime, info->duration, info->user, info->state, info->message, info->transferId);          
}    

#if 1
int Is_ConnectServer(char *host)
{ 
	FILE *stream;
    char recvBuf[16] = {0};
    char cmdBuf[256] = {0};
    int cnt = 4;
    sprintf(cmdBuf, "/etc/ping %s -c %d -i 0.2 | grep time= | wc -l", host, cnt);
    //sprintf(cmdBuf, "/etc/ping %s -c %d | grep time= | wc -l", dst, cnt);
    stream = popen(cmdBuf, "r");
    fread(recvBuf, sizeof(char), sizeof(recvBuf) - 1, stream);
    pclose(stream);
    if(atoi(recvBuf) > 0)
        return 0;

    return -1;
	
}
#endif
