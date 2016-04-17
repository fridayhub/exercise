#include "../act_include.h"

unsigned char temp;
 
//处理KVM_BTN_PC5 按键中断
#pragma interrupt_handler PCINT1_Interrupt:iv_PCINT1
void PCINT1_Interrupt() 
{ 
  	unsigned char kvm_mode;  //读取KVM显示模式
	PCMSK1 = 0x00; //关中断
    temp = PINC;

	if((temp & 0x20) == 0x01) //判断PC5电平
	{
		delay_20mS();
		temp = PINC;
		if((temp & 0x20) == 0x01) //判断PC5电平 延时消抖
		{
			//按键确实被按下 进行KVM切换操作


            /*检测设置开机模式*/
	         kvm_mode = EEPROM_read(KVM_STS_ADDR);  //read kvm mode
			 if (kvm_mode == ACTION_MODEM)  //如果是action一体机 切换成DISP模式
			 {
				KVM_LED1_off;
				KVM_LED2_on;
					
				KVM_USB0_on;
				KVM_USB1_off;
					
				SCALE_SW0_on;
				SCALE_SW1_off;
					
				put_string("For display mode!");
				put_CR();
			 }
			 else  //切换成action一体机模式
			 {
				//LED指示灯
				KVM_LED1_on;
				KVM_LED2_off;
				
				//键鼠切换
				KVM_USB0_off;
				KVM_USB1_on;
				
				//显示器切换
				SCALE_SW0_off;
				SCALE_SW1_on;
				
				put_string("For action modem!");
				put_CR(); 
			 }
		}
	}
  
 PCMSK1 = 0x20; //开中断
} 

//INT0中断 PD2  处理电源按键
#pragma interrupt_handler int0_isr:iv_INT0
void int0_isr(void)
{
    //external interupt on INT0
	EIMSK = 0x02; //关中断INT0
	
	
	EIMSK = 0x03; //开中断INT0
}

//INT1中断 PD3 处理CPU升级
#pragma interrupt_handler int1_isr:iv_INT1
void int1_isr(void)
{
    //external interupt on INT1
	EIMSK = 0x01; //关中断INT1
    PC4_DIS; //PC4 = 0
	PC0_DIS; //PC0 = 0
	PB6_EN;  //PB6 = 1
	PC0_EN;  //PC0 = 1
	delay_3s();
	PB6_EN;  //PB6 = 1
	// 成功 KVM_LED1 闪烁 
    // 失败 KVM_LED2 闪烁
	//结束
	
	EIMSK = 0x03; //开中断INT1
	
}