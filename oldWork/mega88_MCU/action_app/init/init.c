#include "../act_include.h"

extern unsigned char rx_int_flag;
extern unsigned char rx_buf;

void external_interrupt_init(void) //外部中断初始化
{
	MCUCR = 0x00; //中断向量位于flash存储器起始地址处，设置为不可修改
	EICRA = 0x00; //外部中断控制寄存器 A 包括决定中断触发方式的控制位。 INT1、INIT0为低电平时产生中断请求
	EIMSK = 0x03; //外部中断屏蔽寄存器，使能INT1、INT0
	PCICR = 0x07; //引脚电平变化中断控制寄存器,使能PCINT23-0中断，具体引脚由PCMSK1-3单独使能
	PCMSK2 = 0x0C; //引脚电平变化屏蔽寄存器,使能PCINT19(INT10)、PCINT18(INT1)  
	PCMSK1 = 0x20; //引脚电平变化屏蔽寄存器,使能PCINT13(PC5)  按键侦测
	PCMSK0 = 0x00; //引脚电平变化屏蔽寄存器,PCINT7..0 禁用
}

void port_init(void)
{
	 PORTB = 0x00;
	 DDRB  = 0xFF;
	 PORTC = 0x00; //m103 output only
	 DDRC  = 0x1F;
	 PORTD = 0x00;
	 DDRD  = 0xF2;
}

//call this routine to initialize all peripherals
void init_devices(void)
{
	 //stop errant interrupts until set up
	 CLI(); //disable all interrupts
	 port_init();

	 TIMSK0 = 0x00; //timer 0 interrupt sources
	 TIMSK1 = 0x00; //timer 1 interrupt sources
	 TIMSK2 = 0x00; //timer 2 interrupt sources
	 
	 PRR = 0x00; //power controller
	 
	 external_interrupt_init();
	 SEI(); //re-enable interrupts
	 
	 uart_init();
	// timer0_init();
}

//开机模式设置信号中断处理
#pragma interrupt_handler uart0_rx_isr:iv_USART0_RXC
void uart0_rx_isr(void)
{
	rx_buf = UDR0;
	if ((rx_buf == 's') || (rx_buf == 'S'))
	{
		UCSR0B = 0x18;	//disable RX interrrupt
		rx_int_flag = 1;
	}
}