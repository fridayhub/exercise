#ifndef _PROCOTOL_H_
#define _PROCOTOL_H_
#include "../../include.h"
/****************************some macro defines******************************************/
#define RxBufferSize 		1024
#define TxBufferSize 		256
#define MAIN_VERSION 2 //不能更改
#define SUB_VERSION  2  //用户版本号 目前为2

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

/***********************类型标志定义*******************************************************/
#define  DEVICE_STATE        	1	//上传火灾自动报警系统设备状态	上行Device state
#define  PARTS_RUNNING_STATE 	2	//上传火灾自动报警系统部件运行状态	上行 Parts running state
#define  ANALOG_VALUE        	3	//3上传火灾自动报警系统部件模拟量值	上行 Analog
#define  DEVICE_OPER_INFO    	4	//4上传火灾自动报警系统设备操作信息	上行
#define  PASTS_OPER_INFO     	5	//5	上传火灾自动报警系统部件操作信息	上行
#define  DEVICE_SET         	6	//6	上传火灾自动报警系统设备配置情况	上行
#define  PASTS_SET          	7	//7	上传火灾自动报警系统部件配置情况	上行
#define  DVEICE_TIME         	8	//8	上传火灾自动报警系统设备时间	上行
#define  TEST                	9	//9	通信线路上行测试	上行
//10～60	预留	
#define READ_DEVICE_STATE        61 //61读火灾自动报警系统设备状态	下行
#define READ_PARTS_RUNNING_STATE 62//62	读火灾自动报警系统部件运行状态	下行
#define READ_ANALOG_VALUE        63//63	读火灾自动报警系统部件模拟量值	下行
#define READ_DEVICE_SET          64//64	读火灾自动报警系统设备配置情况	下行
#define READ_PASTS_SET           65//65	读火灾自动报警系统部件配置情况	下行
#define READ_DVEICE_TIME         66//66	读火灾自动报警系统设备时间	下行
#define DOWN_TEST                67//67	通信线路下行测试	下行
//68～127	预留	
//128～255	用户自定义	
#define WRITE_DEVICE_IFNO        128//128	写终端设备信息	下行
#define WRITE_LINKAGE_INFO       129//129	写联动公式	下行
#define WRITE_GAS_LINKAGE        130//130	写气体联动公式	下行
#define WRITE_WARN_LINKAGE       131//131	写预警联动公式	下行
#define WRITE_MULT_PANEL         132//132	写多线盘信息	下行
#define WRITE_BUS_PANEL          133//133	写总线盘信息	下行
#define WRITE_DISP_PANEL         134//134	写层显信息	下行
#define WRITE_BROADCAST          135//135	写广播分区信息	下行
#define WRITE_CONT_INFO          136//136	写控制器信息	下行
#define WRITE_LOOP_SET           137//137	写集中机控制器回路数设置	下行
//138~150	自定义预留	
#define READ_DEVICE_INFO         151//151	读终端设备信息	上行
#define READ_LINKAGE_INFO        152//152	读联动公式	上行
#define READ_GAS_LINKAGE         153//153	读气体联动公式	上行
#define READ_WARN_LINKAGE        154//154	读预警联动公式	上行
#define READ_MULT_PANEL          155//155	读多线盘信息	上行
#define READ_BUS_PANEL           156//156	读总线盘信息	上行
#define READ_DISP_PANEL          157//157	读层显信息	上行
#define READ_BROADCAST           158//158	读广播分区信息	上行
#define READ_CONT_INFO           159//159	读控制器信息	上行
#define READ_LOOP_SET            160//160	读集中机控制器回路数设置	上行

#define SET_CONT_OPER            182//182	设置疏散控制器操作(复位、消音、自检、月检、年检) //信息重传

//#define DATASTRUCTSIZE		


/****************************Adapter to Cloud DataStruct******************************************/

typedef struct thingWorxData
{
	tw_Public  tw_SoState;		   	//public function //see include.h
	unsigned int deviceType;
	unsigned int deviceAddr;
	unsigned int componType;			//部件类型
	unsigned int componLoopID;		//部件回路地址
    unsigned int componAddr;      	//部件地址
    unsigned int componChannel;     //部件通道号
	unsigned int componStatus;		//部件状态
	unsigned char occuTime[6];			//发生时间
	unsigned int servicetype;		//调用服务名
    float AnalogValue;
}twData;

typedef struct node
{
    twData data;
    struct node *next;
}listnode, *application_info;

application_info init_rxbuffer_data_list(void); 		
application_info get_rxbuffer_data(application_info head);
void delete_rxbuffer_data(application_info head );
void insert_rxbuffer_data(application_info head , struct thingWorxData data);
void destory_rxbuffer_data_list(application_info head);
void get_catch_file(application_info head);

#endif

