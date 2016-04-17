/*****************************************************
 采用串行接口实现Boot_load应用的实例
 Compiler: ICC-AVR 7.22
 Target: Mega88
 Crystal: 8Mhz
 Used: T/C0,USART0
 *****************************************************/
#include <iom88v.h>
#include <macros.h>
#include <AVRdef.h>      //中断函数头文件

#define SPM_PAGESIZE 64 //M88的一个Flash页为64字节(32字)
#define BAUD 9600 //波特率采用9600bps


#define BAUD_H 0x00
#define BAUD_L 0x0C  
		
//定义全局变量
unsigned char startupString[]="Just Test for app\n\r\0";


 //从RS232发送一个字节
 void uart_putchar(char c)
 { 
      while(!(UCSR0A & 0x20)); 
      UDR0 = c;
 }
 
 //从RS232接收一个字节
 int uart_getchar(void)
 { 
      unsigned char status,res; 
      if(!(UCSR0A & 0x80))
	   return -1; //no data to be received 
      status = UCSR0A; 
      res = UDR0; 
      if (status & 0x1c) 
	   return -1; // If error, return -1 
      return res;
 }
 
 //等待从RS232接收一个有效的字节
 char uart_waitchar(void)
 { 
      int c; 
      while((c = uart_getchar()) == -1); 
      return (char)c;
 }
 

 
//退出Bootloader程序，从0x0000处执行应用程序
 void quit(void)
 { 
      uart_putchar('O');uart_putchar('K'); 
      uart_putchar(0x0d);uart_putchar(0x0a);


	  MCUCR = (1 << IVCE);  //使能中断向量
	  MCUCR = (1 << IVSEL); //将中断向量移到boot区  Boot区起始地址由熔丝位BOOTSZ确定

	 asm("LDI R30,0X00\n"   //LDI 装入立即数
     "LDI R31,0X0C\n"     //z寄存器初始化
	 "ijmp\n");         //跳转到Flash 0x0000处，执行用户应用程序
 }
 
//TIMER0 initialize - prescale:256
// WGM: Normal
// desired value: 1MHz
// actual value:  0.000MHz (800000100.0%)
// 实现20ms延时
void timer0_init(void)
{
 //SREG = 0x80; //使能全局中断
 TCCR0B = 0x00; //stop
 TCNT0 = 0xB3; //set count 0xB2 = 179  4E->78
 //定时器频率=1M/256=3906.25
 //定时器初始值设置，定时时间=（256-178+1）/3906.25=0.020224s=20ms
 TCCR0A = 0x00; 
 TCCR0B = 0x04; //start timer 0x04 ->256分频
 TIMSK0 = 0x01; //enable timer 0 overflow interrupt
}

int counter = 0; //1s计数变量清零
int onesecond = 0; //1s
int my_count = 0;

#pragma interrupt_handler timer0_ovf_isr:iv_TIM0_OVF
void timer0_ovf_isr(void)
{
  TCNT0 = 0xB3; //179
  if(++counter >= 50)   //定时时间1s 20ms*50=1000ms=1s
  {    
    counter = 0;
	onesecond = 1;
  }
}

void port_init(void)
{
 PORTB = 0x00;
 DDRB  = 0x00;
 PORTC = 0x00; //m103 output only
 DDRC  = 0x00;
 PORTD = 0x00;
 DDRD  = 0x00;
}

void uart_init(void)
{
 //初始化M88的USART0 
  UBRR0H = BAUD_H; 
  UBRR0L = BAUD_L; //Set baud rate 
  UCSR0B = 0x18; //Enable Receiver and Transmitter 
  UCSR0C = 0x0E; //Set frame format: 8data, 2stop bit 
  UCSR0A = 0x02;
}

//call this routine to initialize all peripherals
void init_devices(void)
{
 //stop errant interrupts until set up
 CLI(); //disable all interrupts
 port_init();

 MCUCR = 0x00;
 EICRA = 0x00; //extended ext ints
 EIMSK = 0x00;
 
 TIMSK0 = 0x00; //timer 0 interrupt sources
 TIMSK1 = 0x00; //timer 1 interrupt sources
 TIMSK2 = 0x00; //timer 2 interrupt sources
 
 PCMSK0 = 0x00; //pin change mask 0 
 PCMSK1 = 0x00; //pin change mask 1 
 PCMSK2 = 0x00; //pin change mask 2
 PCICR = 0x00; //pin change enable 
 PRR = 0x00; //power controller
 SEI(); //re-enable interrupts
 
 uart_init();
 timer0_init();
}

 //主程序
 void main(void)
 { 
     int i = 0; 
     unsigned char timercount = 0; 
     unsigned char packNO = 1; 
     int bufferPoint = 0; 
     unsigned int crc; 

	 unsigned char *p;
	  
	 init_devices(); //初始化所有状态
	  
	 
     //3秒种等待PC下发"d"，否则退出Bootloader程序，从0x0000处执行应用程序 
     while(1) 
      { 
         if(uart_getchar()== 'd') 
		   quit(); 

		 if(onesecond == 1)  //1s延时
	     {
	       onesecond = 0;
	       my_count++;
	       if(my_count == 2)  //3s延时
	        {
		     my_count = 0;
	         //向PC机发送字符串  
	 		 p = startupString;
             while(*p != '\0') 
              { 
                uart_putchar(*p++); 
              } 
	        }
	      }
      } 
}
