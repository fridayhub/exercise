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


/************************Modbus 功能码****************************************************/
#define ReadCoil           1  //读线圈
#define ReadDiscreteIn     2  //读输入离散量
#define ReadHoldingReg     3   
#define ReadInReg          4  //读输入寄存器
#define WriteSingGoil      5  //写单个线圈
#define WriteSingReg       6  //写单个寄存器
#define WriteMultipleCoils 15 //写多个线圈
#define WriteMultipleReg   16 //写多个寄存器
#define MaskWriteReg       22 //屏蔽写寄存器

/***********************控制单元命令定义*******************************************************/
enum CONTROL_OLDER_DEFINE
{
	CONTROL_OLDER_BK=1,				//预留
	CONTROL_OLDER_DATA,				//发送数据
	CONTROL_OLDER_ACK,				//确认
	CONTROL_OLDER_REQUIRE,			//请求
	CONTROL_OLDER_REPLY,			//应答,返回请求的信息
	CONTROL_OLDER_NAK,				//否认
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

