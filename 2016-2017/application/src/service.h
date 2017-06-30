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
/*					���ö�̬�⺯����					*/
/********************************************************************************************************************/
twInfoTable* CreateNODEInfoTable(DATETIME now,int Num , tw_Public *ptr, int fd); //����������

void * ledBlink(DATETIME now, void *parm);


/**********************************RS232**********************************************************************************/
twInfoTable* (*CreateUARTInfoTable232)(DATETIME  , int , tw_Public *ptr);		//����֧�ֵķ����
int (*InitList232)(int fd);												//��ʼ������ṹ
void (*DestroyList232)(void);
void (*Delete_CurrentNode232)(void);
int (*GetUARTInformation232)(int fd , int port);
int (*SaveList_AsFile232)(void); //�����ļ�

//232 Ŀǰû��Զ�̿���
/***********************************RS485*********************************************************************************/
twInfoTable* (*CreateUARTInfoTable485)(DATETIME  , int , tw_Public *ptr);		//����֧�ֵķ����
int (*InitList485)(int fd);												//��ʼ������ṹ
void (*DestroyList485)(void);
void (*Delete_CurrentNode485)(void);
int (*GetUARTInformation485)(int fd , int port);
void (*readRemoteController485)(int controllerid, int controllertype, int level, char *code,int readType);
void (*setRemoteController485)(int controllerid, int controllertype, int level, char *code); //Զ����Ȩ
void (*RemoteControl485)(int controllerid, int controllertype, int controltype, int controlmarker);//Զ�̿���
void (*setRemoteControlStatus)(int controllerid, int controllertype, int oparatorstatus);//Զ�̿���
int (*SaveList_AsFile485)(void); //�����ļ�

int Is_ConnectServer(char *host);
int GBKToUTF8(const unsigned char* pszBufIn, int nBufInLen, unsigned char* pszBufOut);
void MemCpy(unsigned char *dst , unsigned char *src , int len);
int itoa(unsigned int src , char *dst);
void  itoal(long long i, char*string);
void itoan(unsigned int srNum[], char *dest, int num);
void GetContactStatus(void)	;
#endif
