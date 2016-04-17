#include "../act_include.h"

unsigned char temp;
 
//����KVM_BTN_PC5 �����ж�
#pragma interrupt_handler PCINT1_Interrupt:iv_PCINT1
void PCINT1_Interrupt() 
{ 
  	unsigned char kvm_mode;  //��ȡKVM��ʾģʽ
	PCMSK1 = 0x00; //���ж�
    temp = PINC;

	if((temp & 0x20) == 0x01) //�ж�PC5��ƽ
	{
		delay_20mS();
		temp = PINC;
		if((temp & 0x20) == 0x01) //�ж�PC5��ƽ ��ʱ����
		{
			//����ȷʵ������ ����KVM�л�����


            /*������ÿ���ģʽ*/
	         kvm_mode = EEPROM_read(KVM_STS_ADDR);  //read kvm mode
			 if (kvm_mode == ACTION_MODEM)  //�����actionһ��� �л���DISPģʽ
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
			 else  //�л���actionһ���ģʽ
			 {
				//LEDָʾ��
				KVM_LED1_on;
				KVM_LED2_off;
				
				//�����л�
				KVM_USB0_off;
				KVM_USB1_on;
				
				//��ʾ���л�
				SCALE_SW0_off;
				SCALE_SW1_on;
				
				put_string("For action modem!");
				put_CR(); 
			 }
		}
	}
  
 PCMSK1 = 0x20; //���ж�
} 

//INT0�ж� PD2  �����Դ����
#pragma interrupt_handler int0_isr:iv_INT0
void int0_isr(void)
{
    //external interupt on INT0
	EIMSK = 0x02; //���ж�INT0
	
	
	EIMSK = 0x03; //���ж�INT0
}

//INT1�ж� PD3 ����CPU����
#pragma interrupt_handler int1_isr:iv_INT1
void int1_isr(void)
{
    //external interupt on INT1
	EIMSK = 0x01; //���ж�INT1
    PC4_DIS; //PC4 = 0
	PC0_DIS; //PC0 = 0
	PB6_EN;  //PB6 = 1
	PC0_EN;  //PC0 = 1
	delay_3s();
	PB6_EN;  //PB6 = 1
	// �ɹ� KVM_LED1 ��˸ 
    // ʧ�� KVM_LED2 ��˸
	//����
	
	EIMSK = 0x03; //���ж�INT1
	
}