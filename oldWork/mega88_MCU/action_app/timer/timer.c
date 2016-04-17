#include "../act_include.h"

int counter = 0; //1s计数变量清零
int onesecond = 0; //1s
int my_count = 0;
int i =0;
unsigned char delay_flag;

//TIMER0 initialize - prescale:256
// WGM: Normal
// desired value: 1MHz
// actual value:  0.000MHz (800000100.0%)
// 实现20ms延时
void delay_20mS(void)
{
 //SREG = 0x80; //使能全局中断
 TCCR0B = 0x00; //stop
 TCNT0 = 0xB3; //set count 0xB2 = 179  4E->78
 //定时器频率=1M/256=3906.25
 //定时器初始值设置，定时时间=（256-178+1）/3906.25=0.020224s=20ms
 TCCR0A = 0x00; 
 TCCR0B = 0x04; //start timer 0x04 ->256分频
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
  if(++counter >= 50)   //定时时间1s 20ms*50=1000ms=1s
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