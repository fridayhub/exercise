#include "include.h"
#include "service.h"
#include <linux/input.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <regex.h>
#include <poll.h>
#include <stropts.h>
#include <errno.h>
static volatile int AlarmStatus=0;			
static volatile unsigned char	AlarmMASK=0;		//模块报警状态
const char *CONTACT_PATH = "/dev/input/event1";   

const char ALARM_FIRE = 107;
const char ALARM_FAULT = 108;

extern char dynamicName232[];
extern int alarm_state;
void GetContactStatus(void)		//同一状态只上报一次
{
	int ret;
  	struct input_event tw_dev;
	struct timeval tv;
	fd_set fds;
	int i, fd;

	fd = open(CONTACT_PATH, O_RDONLY | O_NOCTTY | O_NDELAY);
    //printf("open %s %s %d \n", __FILE__, __func__, __LINE__);
	if(fd<0)	
	{  
		perror("open /dev/input/event1\n");  
        return -1;
	}

	FD_ZERO(&fds);
	FD_SET(fd , &fds);
	while(1)
	{
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		ret = select(fd+1 , &fds , NULL , NULL , &tv);
		if(ret<0)
		{
			continue;
		}
		else if(ret == 0)
		{
			AlarmMASK=0;
			if(AlarmStatus&0x01)//之前产生过火警
			{
				AlarmMASK|=0x02;//火警解除
			}
			if(AlarmStatus&0x10)//之前产生过故障
			{
				AlarmMASK|=0x20;//火警解除
			}
			AlarmStatus=0;
			printf("\ncontact read time out\n");  
            close(fd);
			return;
		}
		else if(FD_ISSET(fd , &fds))
		{
			if(read(fd , &tw_dev , sizeof(tw_dev))==sizeof(tw_dev))
			{
				if(tw_dev.type==EV_KEY&&tw_dev.code==ALARM_FIRE)
				{
					if((AlarmStatus&0x01)==0)		//按下
					{
						printf("\n***keyvalA***\n");
						AlarmMASK |= 0x01;
						AlarmStatus|=0x01;
						alarm_state |= 0x01;
					}
					else
						break;
				}
				if(tw_dev.type==EV_KEY&&tw_dev.code==ALARM_FAULT)
				{
					if((AlarmStatus&0x10)==0)		//按下
					{
						printf("\n***keyvalB**\n");
						AlarmMASK |= 0x10;
						AlarmStatus|=0x10;
						alarm_state |= 0x10;
					}
					else
						break;
				}
			}
			else
			{
				close(fd);
				return;
			}
		}
	}
    close(fd);
    return;
}
	

twInfoTable* CreateNODEInfoTable(DATETIME now,int Num , tw_Public *ptr, int fd)
{
	/* Make the request to the server */
	
	twInfoTable * content = NULL;
	twInfoTableRow *row = 0;
	twDataShape * ds = 0;
	char now_time[25];
	long long linuxTime = 0;
	int ret;
	int tw_alarm=0;	
    unsigned char controllerID[8] = {0}, deviceStatus[8] = {0}, controllerType[5] = {0}, deviceType[5] = {0},  operatorMark[5] = {0}, operatornum[5] = {0}, deviceAddr[30] = {0};
	
	ptr->ServerType=SERVER_BK;
	#if 1
	if(AlarmMASK==0)	
	{
		return NULL;
	}

	ds = twDataShape_Create(twDataShapeEntry_Create("ControllerID", NULL, TW_STRING));
	if (!ds) 
	{
		TW_LOG(TW_ERROR, "Error Creating datashape.");
		return NULL;
	}
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("Flag", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("ControllerType", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceType", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("deviceNumber", NULL, TW_STRING));  //二次码方式
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("DeviceState", NULL, TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("CurrentTime", NULL, TW_STRING));
	content = twInfoTable_Create(ds);
	if (!content) 
	{
		TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable");
		twDataShape_Delete(ds); 
		return NULL;
	}
	itoal(now, now_time);	
	//printf("\n\n\nthe board time is %s, now is%llu\n\n\n\n", now_time, now);
	row = twInfoTableRow_Create(twPrimitive_CreateFromString(controllerID, TRUE));
	if (!row) 
	{
		TW_LOG(TW_ERROR, "getSteamSensorReadingsService - Error creating infotable row");
		twInfoTable_Delete(content);	
		return NULL;
	}
	
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("0", TRUE));			   // FLAG	2:232, 3:485
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(controllerType, TRUE));			 //ControllerType
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceType, TRUE));			//DeviceType
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(deviceAddr, TRUE));	   //二次码
	if(AlarmMASK&0x01)
	{
		alarm_state |= 0x01;
		AlarmMASK&= ~0x01;
		twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("1", TRUE)); 		   //火警	
	}
	else if(AlarmMASK&0x02)
	{
		AlarmMASK&= ~0x02;
		twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("13", TRUE)); 		   //火警解除
	}
	else if(AlarmMASK&0x10)
	{
		alarm_state |= 0x10;
		AlarmMASK&= ~0x10;
		twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("20", TRUE)); 		   //故障	
	}
	else if(AlarmMASK&0x20)
	{
		AlarmMASK&= ~0x20;
		twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("40", TRUE)); 		   //故障解除
	}
	
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(now_time, TRUE));		  //time
	/* add the InfoTableRow to the InfoTable */
	twInfoTable_AddRow(content, row);
	
	ptr->ServerType = SET_DEVICE_STATE;
	return content;
	#endif
}

