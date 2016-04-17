#include "../act_include.h"

int counter = 0; //1s������������
int onesecond = 0; //1s
int my_count = 0;
int i =0;
unsigned char delay_flag;

//TIMER0 initialize - prescale:256
// WGM: Normal
// desired value: 1MHz
// actual value:  0.000MHz (800000100.0%)
// ʵ��20ms��ʱ
void delay_20mS(void)
{
 //SREG = 0x80; //ʹ��ȫ���ж�
 TCCR0B = 0x00; //stop
 TCNT0 = 0xB3; //set count 0xB2 = 179  4E->78
 //��ʱ��Ƶ��=1M/256=3906.25
 //��ʱ����ʼֵ���ã���ʱʱ��=��256-178+1��/3906.25=0.020224s=20ms
 TCCR0A = 0x00; 
 TCCR0B = 0x04; //start timer 0x04 ->256��Ƶ
 TIMSK0 = 0x01; //enable timer 0 overflow interrupt
 
 while (delay_flag == 0); 
 delay_flag = 0;
 
 TCCR0B = 0x00; //stop
 TIMSK0 = 0x00; //disable timer 0 overflow interrupt
}

#pragma interrupt_handler timer0_ovf_isr:iv_TIM0_OVF
void timer0_ovf_isr(void)
{
  delay_flag = 1;
  /*TCNT0 = 0xB3; //179
  if(++counter >= 50)   //��ʱʱ��1s 20ms*50=1000ms=1s
  {    
    counter = 0;
	onesecond = 1;
  }*/
}

void delay_1s(void)
{
    for(i = 0; i < 50; i++)
		delay_20mS();
}

void delay_3s(void)
{
    for(i = 0; i < 150; i++)
		delay_20mS();
}