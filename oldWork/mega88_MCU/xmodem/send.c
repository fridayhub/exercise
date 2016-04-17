/*
 * 功能：通过串口，用xmodem协议发送文件。
 * 
 * 说明：运行程序，按提示输入数字选择相应功能
 *
 * 其他：功能有待完善，如添加接收和聊天功能
 *
 * 日期：2011-4-23
 */
#include"send.h"
int main(int argc,char *argv[])
{
	int fd;
	char *data_file_name;
	char packet_data[XMODEM_DATA_SIZE];
	char frame_data[XMODEM_DATA_SIZE + XMODEM_CRC_SIZE + XMODEM_FRAME_ID_SIZE + 1];
	unsigned char tmp;
	unsigned char num,num1;
	char  name1[100]={};
	char sr,ss,cr,cs;
	char kk;
	int fdcom;
	int RecvLen,RecvLen1;

	FILE *datafile;
	FILE *received;
	int complete,retry_num,pack_counter,read_number,write_number,i;
	unsigned short crc_value;
	unsigned char ack_id;
	char  name[100]={};
	char  receivefilename[100];
	char RecvBuf[1000];
	char RecvBuf1[1000];
	
	if(argc!=1)
	{
		printf("Usage:%s\n",argv[0]);
		printf("eg:");
		printf("./serial\n");
		exit(-1);
	}
	else
	{
		printf("XMODEM started...\r\n");
	}
	/* open serial port */
	if ((fd = Initial_SerialPort()) == -1)  
	{
		printf("Can't open ttyUSB0\n");
		return -1 ;
    }
	
	while(1)
	{
			sr='\0';
			printf("please choose: 1-send || 2-receive || 3-exit!\n");
			sr=getchar();
			ss=getchar();
			
			kk=1;
		while(kk)
		{
		  pack_counter = 0;	// 包计数器清零
		  complete = 0;
		  retry_num = 0;
		  ClearReceiveBuffer(fd);//等待串口恢复空闲状态，发送数据则一直等待，直到读不到串口发送的数据，退出循环等待
          if(sr=='1')
	         {
	            printf("please input the sendfile path and name:\n");
				printf("eg:/home/usr/work/serial/send.txt\n");
				while((cr=getchar())!='\n')
				{
					receivefilename[num]=cr;
					num++;
				}
				
				for(i=0;i<num;i++)
					name[i]=receivefilename[i];
					num=0;
				//只读方式打开一个准备发送的文件，如果不存在就报错，退出程序。	
			    if((datafile=fopen(name,"rb"))==NULL)
				{
					printf("\n can't open this file or not exist! \n");
					return -1;
				}
				else
				{
				    printf("Ready to send the file:%s\n...\n......\n",name);
				}
				bzero(&name,sizeof(name));
	            printf("Waiting for singnal NAK!\n...\n......\n");
				while((read(fd,&ack_id,1)) <= 0);//从串口读一个字节(NAK信号)到 ack_id 中，读不到数据则等待，直到读到数据后退出循环
				printf("The Singnal NAK: %c ok!!!\n",ack_id);//打印接收到的NAK信息
				while(!complete)
				{
					switch(ack_id)
					{
						case XMODEM_CRC_CHR:	// 接收到字符'C'开始启动传输，并使用CRC校验
							printf("begining to Send file %s...\n",name);

						case XMODEM_ACK:        //0x06
							retry_num = 0;
							pack_counter++;

                            printf("XMODEM_DATA_SIZE is %d\n", XMODEM_DATA_SIZE);
							read_number = fread(packet_data, sizeof(char), XMODEM_DATA_SIZE, datafile);
										//从打开的datafile指向的文件中读取 
										//XMODEM_DATA_SIZE 个（char）数据，
										//放到packet_data这个数组中
							if(read_number > 0)//read_number为返回的读取实际字节数
							{
								if(read_number < XMODEM_DATA_SIZE_SOH)
								{
									printf("Start filling the last frame!\r\n");
									for(; read_number < XMODEM_DATA_SIZE; read_number++)
									packet_data[read_number] = 0x1A;  // 不足128字节用0x1A填充
								}

								frame_data[0] = XMODEM_HEAD;  // 帧开始字符
								frame_data[1] = (char)pack_counter;  // 信息包序号
								frame_data[2] = (char)(255 - frame_data[1]);  // 信息包序号的补码
	
								for(i=0; i < XMODEM_DATA_SIZE; i++)  // 128字节的数据段
								frame_data[i+3] = packet_data[i];//把收到的字符和信息头一起打包
	
								crc_value = GetCrc16(packet_data, XMODEM_DATA_SIZE); // 16位crc校验
								frame_data[XMODEM_DATA_SIZE_SOH+3] = (unsigned char)(crc_value >> 8);// 高八位数据
								frame_data[XMODEM_DATA_SIZE_SOH+4] = (unsigned char)(crc_value);     //低八位数据

								/* 发送133字节数据 */
								write_number = write( fd, frame_data, XMODEM_DATA_SIZE_SOH + 5);//向串口写一个包数据，即133字节数据

								printf("The %d pack(frame size:%d)has been sended!!\n",pack_counter,write_number);
								printf("waiting for next ACK... \n......\n");
								while((read(fd,&ack_id,1)) <= 0);
			
								if(ack_id == XMODEM_ACK)
								printf("ACK Ok!!Ready sending next pack!\r\n");
								else
								{
									printf("ACK Error!\r\n");
									printf("%c\r\n",ack_id);
								}
							}

						else  // 文件发送完成
						{
							ack_id = XMODEM_EOT;
							complete = 1;
							printf("Complete ACK have coming,");
			
							while(ack_id != XMODEM_ACK)
							{
								ack_id = XMODEM_EOT;	
								write_number = write(fd,&ack_id,1);
								while((read(fd, &ack_id, 1)) <= 0);
							}
							printf("Send file successful!!!\r\n");
							kk=0;
							fclose(datafile);
						}
						break;

						case XMODEM_NAK:
						if( retry_num++ > 10) 
						{
							printf("Retry too many times,Quit!\r\n");
							complete = 1;
						}
						else //重试，发送
						{
							write_number = write(fd, frame_data, XMODEM_DATA_SIZE + 5);
							printf("Retry for ACK,%d,%d...", pack_counter, write_number);

							while((read(fd, &ack_id, 1)) <= 0);
				
							if( ack_id == XMODEM_ACK )
							printf("OK\r\n");
							else
							printf("Error!\r\n");
						}
						break;
						default:
							printf("Fatal Error!\r\n");
							complete = 1;
						break;
					}
				}
            
			}
			else if(sr=='2')
			{
				printf("please input the receivefile path and name:\n");
				printf("eg:/home/usr/work/serial/receive.txt\n");
				while((cr=getchar())!='\n')
				{
					receivefilename[num1]=cr;
					num1++;
				}
				
				for(i=0;i<num1;i++)
					name1[i]=receivefilename[i];
					num1=0;
				//打开一个接收文件，如果不存在就创建它！若失败则提示错误，退出程序。	
			    if((received=fopen(name1,"w+"))==NULL)
				{
					printf("\n can't creat receivefile %s! \n",name1);
					return -1;
				}
				else
				{
				    printf("creat receivefile:%s success!\n",name1);
					printf("Waiting for receive file....\n.....\n.........\n");
				}
				bzero(&name1,sizeof(name1));
				//ack_id='c';
				//while((read(fd, &cs, 1)) <= 0);
				//printf("cs is %c\n",cs);
				RecvLen=PortRecv(fd,RecvBuf,1,115200);//从串口接收数据
				printf("%d\n",RecvLen);	
					      if(RecvLen>1&&(RecvBuf[0]!='\n'))
							{						
								for(i=1;i<=RecvLen;i++)   //把接收到的数据写入文件
								{
									fputc(RecvBuf[i-1],received);
								}
							}	
								printf("Receive flie complete!\n");
							     fclose(received);      //写入完毕，关闭文件
								printf("\n");
								kk=0;
								sr='\0';	
			}
			else if(sr=='3')
			{
				    printf("Byebye!!\n"); //退出！
					close(fd);
					return 0;
			}
			else
			{
				kk=0;
				printf("choose error:     %c!!\n",sr);	
			}
			
		}
	
	}
	fclose(datafile);
	close(fd);
	kk=0;
	return 0;
}
/*****************************************************************************/
