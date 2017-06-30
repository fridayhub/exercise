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


//服务类型定义
/***************************其他厂家类型编码******************************************************/
#define SERVER_BK				0XFFFF	//	保留
#define SIEMENS_JB_TGZL_FC18R	0XFFFE	//	设置西门子打印机上传云台服务setDeviceState2DBByxmz
#define SET_CONTROLLER_STATE	0x01	//	控制器本身状态
#define SET_DEVICE_STATE		0x02	//	建筑消防设施部件运行状态
#define SET_OPERATOR_INFO		0x04  	//	设置消防设施操作信息
#define REMOTE_ENABLE					71    //远程授权
#define READ_REMOTE_LEVEL_STATUS		72    //读远程授权状态
#define READ_CONTROLLER_ID				73    //读控制器系统ID
#define SET_REMOTE_ENABLE_STATUS		74    //设置远程控制使能状态
#define READ_REMOTE_ENABLE_STATUS		75    //读远程控制使能状态
#define REMOTE_CONTROLLER				76    //远程控制
#define READ_REMOTE_STATUS				77    //读取远程控制状态
#define WATER_UPLOAD                    100   //上传水系统模拟量数据
#define UPDATE_WATER_CONFIG             101   //更新配置数据


//远程控制定义
/***************************远程控制标识******************************************************/
#define REMOTE_NO				0XFFFF		//	没有远程控制
#define REMOTE_UP_LENVEL		0x01		//	上传授权等级
#define REMOTE_UP_CONTROL_UDID	0x02		//	上传控制器识别码
#define REMOTE_UP_STATUS		0x03		//	上传远程控制状态信息
/*This Struct every .so lib should have , main task use it for get date state and server*/
typedef struct 
{
	unsigned int ServerType;
	char dynamicName232[50];
	char dynamicName485[50];
}tw_Public;
long int getConvertTime(char *occurTime);

#endif




