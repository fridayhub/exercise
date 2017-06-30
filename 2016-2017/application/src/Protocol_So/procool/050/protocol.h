#ifndef _PROCOTOL_H_
#define _PROCOTOL_H_
#include "./../../../include.h"
/****************************some macro defines******************************************/
#define RxBufferSize 		1024
#define TxBufferSize 		256

/****************************Adapter to Cloud DataStruct******************************************/
typedef struct twCenterData{
    unsigned int componLoopID;          //��·��ַ
    unsigned int deviceAddr;            //�豸��ַ
    unsigned int infoType;              //��������
    unsigned int extensionNum;          //�ֻ���
    unsigned char occurTime[6];          //����ʱ��
    int serverType;                     //��Ҫ���õķ����־λ
    int dataFlag;                       //1 �˽ṹ���������ʧ��
}twData;

typedef struct node
{
    twData data;
    struct node *next;
}listnode, *application_info;

application_info init_rxbuffer_data_list(void); 		
application_info get_rxbuffer_data(application_info head);
void delete_rxbuffer_data(application_info head );
void insert_rxbuffer_data(application_info head , struct twCenterData data);
int linklist_is_empty(application_info head);
void destory_rxbuffer_data_list(application_info head);
int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen);

#endif

