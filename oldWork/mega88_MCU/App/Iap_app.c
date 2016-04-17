/*****************************************************
 ���ô��нӿ�ʵ��Boot_loadӦ�õ�ʵ��
 Compiler: ICC-AVR 7.22
 Target: Mega88
 Crystal: 8Mhz
 Used: T/C0,USART0
 *****************************************************/
#include <iom88v.h>
#include <macros.h>
#include <AVRdef.h>      //�жϺ���ͷ�ļ�

#define SPM_PAGESIZE 64 //M88��һ��FlashҳΪ64�ֽ�(32��)
#define BAUD 9600 //�����ʲ���9600bps


#define BAUD_H 0x00
#define BAUD_L 0x0C  
		
//����ȫ�ֱ���
unsigned char startupString[]="Just Test for app\n\r\0";


 //��RS232����һ���ֽ�
 void uart_putchar(char c)
 { 
      while(!(UCSR0A & 0x20)); 
      UDR0 = c;
 }
 
 //��RS232����һ���ֽ�
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
 
 //�ȴ���RS232����һ����Ч���ֽ�
 char uart_waitchar(void)
 { 
      int c; 
      while((c = uart_getchar()) == -1); 
      return (char)c;
 }
 

 
//�˳�Bootloader���򣬴�0x0000��ִ��Ӧ�ó���
 void quit(void)
 { 
      uart_putchar('O');uart_putchar('K'); 
      uart_putchar(0x0d);uart_putchar(0x0a);


	  MCUCR = (1 << IVCE);  //ʹ���ж�����
	  MCUCR = (1 << IVSEL); //���ж������Ƶ�boot��  Boot����ʼ��ַ����˿λBOOTSZȷ��

	 asm("LDI R30,0X00\n"   //LDI װ��������
     "LDI R31,0X0C\n"     //z�Ĵ�����ʼ��
	 "ijmp\n");         //��ת��Flash 0x0000����ִ���û�Ӧ�ó���
 }
 
//TIMER0 initialize - prescale:256
// WGM: Normal
// desired value: 1MHz
// actual value:  0.000MHz (800000100.0%)
// ʵ��20ms��ʱ
void timer0_init(void)
{
 //SREG = 0x80; //ʹ��ȫ���ж�
 TCCR0B = 0x00; //stop
 TCNT0 = 0xB3; //set count 0xB2 = 179  4E->78
 //��ʱ��Ƶ��=1M/256=3906.25
 //��ʱ����ʼֵ���ã���ʱʱ��=��256-178+1��/3906.25=0.020224s=20ms
 TCCR0A = 0x00; 
 TCCR0B = 0x04; //start timer 0x04 ->256��Ƶ
 TIMSK0 = 0x01; //enable timer 0 overflow interrupt
}

int counter = 0; //1s������������
int onesecond = 0; //1s
int my_count = 0;

#pragma interrupt_handler timer0_ovf_isr:iv_TIM0_OVF
void timer0_ovf_isr(void)
{
  TCNT0 = 0xB3; //179
  if(++counter >= 50)   //��ʱʱ��1s 20ms*50=1000ms=1s
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
 //��ʼ��M88��USART0 
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

 //������
 void main(void)
 { 
     int i = 0; 
     unsigned char timercount = 0; 
     unsigned char packNO = 1; 
     int bufferPoint = 0; 
     unsigned int crc; 

	 unsigned char *p;
	  
	 init_devices(); //��ʼ������״̬
	  
	 
     //3���ֵȴ�PC�·�"d"�������˳�Bootloader���򣬴�0x0000��ִ��Ӧ�ó��� 
     while(1) 
      { 
         if(uart_getchar()== 'd') 
		   quit(); 

		 if(onesecond == 1)  //1s��ʱ
	     {
	       onesecond = 0;
	       my_count++;
	       if(my_count == 2)  //3s��ʱ
	        {
		     my_count = 0;
	         //��PC�������ַ���  
	 		 p = startupString;
             while(*p != '\0') 
              { 
                uart_putchar(*p++); 
              } 
	        }
	      }
      } 
}
