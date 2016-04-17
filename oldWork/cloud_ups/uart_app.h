#ifndef _UART_MSG_H_
#define _UART_MSG_H_


//uart request cmmond type
#define UART_CMD_GET_IP       0x01
#define UART_CMD_SET_IP       0x02
//#define UART_CMD_GET_PORT       0x03
//#define UART_CMD_SET_PORT       0x04
#define UART_CMD_GET_HELP       0x05
#define UART_CMD_VIEW_SET       0x06
#define UART_CMD_SAVE_SET      0x0B
#define UART_CMD_CANCEL_SAVE   0x0C
//uart ack cmmond type
#define UART_ACK_IP        0x07
#define UART_ACK_PORT      0x08
#define UART_ACK_HELP      0x09
#define UART_ACK_SET       0x0A
#define UART_ACK_OK       0x03
#define UART_ACK_ER       0x04

//uart help message
//#define UART_HELP_MSG "Uart Usage:                        \
                       0. Get Help                        \
                       1. Set IP(192.168.0.190：12345)    \
                       2. Get IP                          \
                       3. View Set                        \
                       4. Save Set                        \
                       5. Cancel Save                     \
                       "
char UART_HELP_MSG[] = {"Uart Usage:\n0. Get Help\n1. Set IP(192.168.0.190：12345)\n2. Get IP\n3. View Set\n4. Save Set\n5. Cancel Save\n"};

char help[][100]={ "Commands:\n",
                   "setip  - Set the ip and port to ups device \n",
                   "setdelay  - Set the delay time (s) to alarm \n"
              };
#endif //_UART_MSG_H_
