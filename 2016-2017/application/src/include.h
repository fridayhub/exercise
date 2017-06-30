#ifndef _INCLUDE_H_
#define _INCLUDE_H_

#include "twOSPort.h"
#include "twLogger.h"
#include "twApi.h"
#include "twThreads.h"
#include "twFileManager.h"
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/serial.h>
#include <dlfcn.h> 
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "uart.h"
/***************************Macro Define***************************************************/
#define PORT_RS232			1
#define PORT_RS485			2
/***************************Macro Define***************************************************/


//�������Ͷ���
/***************************�����������ͱ���******************************************************/
#define SERVER_BK				0XFFFF	//	����
#define SIEMENS_JB_TGZL_FC18R	0XFFFE	//	���������Ӵ�ӡ���ϴ���̨����setDeviceState2DBByxmz
#define SET_CONTROLLER_STATE	0x01	//	����������״̬
#define SET_DEVICE_STATE		0x02	//	����������ʩ��������״̬
#define SET_OPERATOR_INFO		0x04  	//	����������ʩ������Ϣ
#define REMOTE_ENABLE					71    //Զ����Ȩ
#define READ_REMOTE_LEVEL_STATUS		72    //��Զ����Ȩ״̬
#define READ_CONTROLLER_ID				73    //��������ϵͳID
#define SET_REMOTE_ENABLE_STATUS		74    //����Զ�̿���ʹ��״̬
#define READ_REMOTE_ENABLE_STATUS		75    //��Զ�̿���ʹ��״̬
#define REMOTE_CONTROLLER				76    //Զ�̿���
#define READ_REMOTE_STATUS				77    //��ȡԶ�̿���״̬
#define WATER_UPLOAD                    100   //�ϴ�ˮϵͳģ��������
#define UPDATE_WATER_CONFIG             101   //������������


//Զ�̿��ƶ���
/***************************Զ�̿��Ʊ�ʶ******************************************************/
#define REMOTE_NO				0XFFFF		//	û��Զ�̿���
#define REMOTE_UP_LENVEL		0x01		//	�ϴ���Ȩ�ȼ�
#define REMOTE_UP_CONTROL_UDID	0x02		//	�ϴ�������ʶ����
#define REMOTE_UP_STATUS		0x03		//	�ϴ�Զ�̿���״̬��Ϣ
/*This Struct every .so lib should have , main task use it for get date state and server*/
typedef struct 
{
	unsigned int ServerType;
	char dynamicName232[50];
	char dynamicName485[50];
}tw_Public;
long int getConvertTime(char *occurTime);

#endif




