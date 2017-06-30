#ifndef _SERVICE_H_
#define _SERVICE_H_
#include  "include.h"
extern char AdaptorID[100];


struct  propertyDefinitions
{
    double ID;
    char type[255];
    int protocolType;
    char belongProjectName[255];
} properties;

void sendPropertyUpdate(void);

void fileCallbackFunc (char fileRcvd, twFileTransferInfo * info, void * userdata);
void analysisOptions(void);
void remoteContorller(void);
enum msgCodeEnum propertyHandler(const char * entityName, const char * propertyName,  twInfoTable ** value, char isWrite, void * userdata);
enum msgCodeEnum getPlatformtime(const char * entityName, const char * serviceName, twInfoTable * params, twInfoTable ** content, void * userdata);
void AuthEventHandler(char * credType, char * credValue, void * userdata);
void BindEventHandler(char * entityName, char isBound, void * userdata);


/********************************************************************************************************************/
/*					调用动态库函数表					*/
/********************************************************************************************************************/
twInfoTable* CreateNODEInfoTable(DATETIME now,int Num , tw_Public *ptr, int fd); //适配器触点

void * ledBlink(DATETIME now, void *parm);


/**********************************RS232**********************************************************************************/
twInfoTable* (*CreateUARTInfoTable232)(DATETIME  , int , tw_Public *ptr);		//创建支持的服务表
int (*InitList232)(int fd);												//初始化链表结构
void (*DestroyList232)(void);
void (*Delete_CurrentNode232)(void);
int (*GetUARTInformation232)(int fd , int port);
int (*SaveList_AsFile232)(void); //缓存文件

//232 目前没有远程控制
/***********************************RS485*********************************************************************************/
twInfoTable* (*CreateUARTInfoTable485)(DATETIME  , int , tw_Public *ptr);		//创建支持的服务表
int (*InitList485)(int fd);												//初始化链表结构
void (*DestroyList485)(void);
void (*Delete_CurrentNode485)(void);
int (*GetUARTInformation485)(int fd , int port);
void (*readRemoteController485)(int controllerid, int controllertype, int level, char *code,int readType);
void (*setRemoteController485)(int controllerid, int controllertype, int level, char *code); //远程授权
void (*RemoteControl485)(int controllerid, int controllertype, int controltype, int controlmarker);//远程控制
void (*setRemoteControlStatus)(int controllerid, int controllertype, int oparatorstatus);//远程控制
int (*SaveList_AsFile485)(void); //缓存文件

int Is_ConnectServer(char *host);
int GBKToUTF8(const unsigned char* pszBufIn, int nBufInLen, unsigned char* pszBufOut);
void MemCpy(unsigned char *dst , unsigned char *src , int len);
int itoa(unsigned int src , char *dst);
void  itoal(long long i, char*string);
void itoan(unsigned int srNum[], char *dest, int num);
void GetContactStatus(void)	;
#endif
