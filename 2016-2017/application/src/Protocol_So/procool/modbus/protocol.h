#ifndef _PROCOTOL_H_
#define _PROCOTOL_H_
#include "./../../../include.h"

/* define base type*/
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

/****************************some macro defines******************************************/
#define RxBufferSize 		1024
#define TxBufferSize 		256


/************************Modbus ������****************************************************/
#define ReadCoil           1  //����Ȧ
#define ReadDiscreteIn     2  //��������ɢ��
#define ReadHoldingReg     3   
#define ReadInReg          4  //������Ĵ���
#define WriteSingGoil      5  //д������Ȧ
#define WriteSingReg       6  //д�����Ĵ���
#define WriteMultipleCoils 15 //д�����Ȧ
#define WriteMultipleReg   16 //д����Ĵ���
#define MaskWriteReg       22 //����д�Ĵ���

/***********************���Ƶ�Ԫ�����*******************************************************/
enum CONTROL_OLDER_DEFINE
{
	CONTROL_OLDER_BK=1,				//Ԥ��
	CONTROL_OLDER_DATA,				//��������
	CONTROL_OLDER_ACK,				//ȷ��
	CONTROL_OLDER_REQUIRE,			//����
	CONTROL_OLDER_REPLY,			//Ӧ��,�����������Ϣ
	CONTROL_OLDER_NAK,				//����
};


/****************************Adapter to Cloud DataStruct******************************************/

typedef struct twWaterData
{
	tw_Public  tw_SoState;		   	//public function //see include.h
    int channelValue;
    double analogValue;
    unsigned int servicetype;
}waterData;


typedef struct node
{
    waterData data;
    struct node *next;
}listnode, *application_info;

application_info init_rxbuffer_data_list(void); 		
application_info get_rxbuffer_data(application_info head);
void delete_rxbuffer_data(application_info head );
void insert_rxbuffer_data(application_info head , struct twWaterData data);
void destory_rxbuffer_data_list(application_info head);
void get_catch_file(application_info head);

#endif

