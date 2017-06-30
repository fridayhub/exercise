#ifndef _PROCOTOL_H_
#define _PROCOTOL_H_
#include "../../include.h"
/****************************some macro defines******************************************/
#define RxBufferSize 		1024
#define TxBufferSize 		256
#define MAIN_VERSION 2 //���ܸ���
#define SUB_VERSION  2  //�û��汾�� ĿǰΪ2

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

/***********************���ͱ�־����*******************************************************/
#define  DEVICE_STATE        	1	//�ϴ������Զ�����ϵͳ�豸״̬	����Device state
#define  PARTS_RUNNING_STATE 	2	//�ϴ������Զ�����ϵͳ��������״̬	���� Parts running state
#define  ANALOG_VALUE        	3	//3�ϴ������Զ�����ϵͳ����ģ����ֵ	���� Analog
#define  DEVICE_OPER_INFO    	4	//4�ϴ������Զ�����ϵͳ�豸������Ϣ	����
#define  PASTS_OPER_INFO     	5	//5	�ϴ������Զ�����ϵͳ����������Ϣ	����
#define  DEVICE_SET         	6	//6	�ϴ������Զ�����ϵͳ�豸�������	����
#define  PASTS_SET          	7	//7	�ϴ������Զ�����ϵͳ�����������	����
#define  DVEICE_TIME         	8	//8	�ϴ������Զ�����ϵͳ�豸ʱ��	����
#define  TEST                	9	//9	ͨ����·���в���	����
//10��60	Ԥ��	
#define READ_DEVICE_STATE        61 //61�������Զ�����ϵͳ�豸״̬	����
#define READ_PARTS_RUNNING_STATE 62//62	�������Զ�����ϵͳ��������״̬	����
#define READ_ANALOG_VALUE        63//63	�������Զ�����ϵͳ����ģ����ֵ	����
#define READ_DEVICE_SET          64//64	�������Զ�����ϵͳ�豸�������	����
#define READ_PASTS_SET           65//65	�������Զ�����ϵͳ�����������	����
#define READ_DVEICE_TIME         66//66	�������Զ�����ϵͳ�豸ʱ��	����
#define DOWN_TEST                67//67	ͨ����·���в���	����
//68��127	Ԥ��	
//128��255	�û��Զ���	
#define WRITE_DEVICE_IFNO        128//128	д�ն��豸��Ϣ	����
#define WRITE_LINKAGE_INFO       129//129	д������ʽ	����
#define WRITE_GAS_LINKAGE        130//130	д����������ʽ	����
#define WRITE_WARN_LINKAGE       131//131	дԤ��������ʽ	����
#define WRITE_MULT_PANEL         132//132	д��������Ϣ	����
#define WRITE_BUS_PANEL          133//133	д��������Ϣ	����
#define WRITE_DISP_PANEL         134//134	д������Ϣ	����
#define WRITE_BROADCAST          135//135	д�㲥������Ϣ	����
#define WRITE_CONT_INFO          136//136	д��������Ϣ	����
#define WRITE_LOOP_SET           137//137	д���л���������·������	����
//138~150	�Զ���Ԥ��	
#define READ_DEVICE_INFO         151//151	���ն��豸��Ϣ	����
#define READ_LINKAGE_INFO        152//152	��������ʽ	����
#define READ_GAS_LINKAGE         153//153	������������ʽ	����
#define READ_WARN_LINKAGE        154//154	��Ԥ��������ʽ	����
#define READ_MULT_PANEL          155//155	����������Ϣ	����
#define READ_BUS_PANEL           156//156	����������Ϣ	����
#define READ_DISP_PANEL          157//157	��������Ϣ	����
#define READ_BROADCAST           158//158	���㲥������Ϣ	����
#define READ_CONT_INFO           159//159	����������Ϣ	����
#define READ_LOOP_SET            160//160	�����л���������·������	����

#define SET_CONT_OPER            182//182	������ɢ����������(��λ���������Լ졢�¼졢���) //��Ϣ�ش�

//#define DATASTRUCTSIZE		


/****************************Adapter to Cloud DataStruct******************************************/

typedef struct thingWorxData
{
	tw_Public  tw_SoState;		   	//public function //see include.h
	unsigned int deviceType;
	unsigned int deviceAddr;
	unsigned int componType;			//��������
	unsigned int componLoopID;		//������·��ַ
    unsigned int componAddr;      	//������ַ
    unsigned int componChannel;     //����ͨ����
	unsigned int componStatus;		//����״̬
	unsigned char occuTime[6];			//����ʱ��
	unsigned int servicetype;		//���÷�����
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

