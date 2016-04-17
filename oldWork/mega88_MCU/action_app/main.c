#include "act_include.h"

unsigned char pwrup_sts;
unsigned char rx_int_flag = 0;  //获取串口信号，用来设置开机模式
unsigned char rx_buf;
 
extern int counter; //1s计数变量清零
extern int onesecond; //1s
extern int my_count;
 
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

void setup(void)
{
	unsigned char call_sts = 1;
	//stop_timer0();

	while (call_sts)
	{
		if (setup_pwrup_mode() != 1) call_sts = 1;
		else call_sts = 0;
	}

	put_string("Setup power mode is success, pls re power up system!"); put_CR(); put_CR();

	while (1);
}

unsigned char setup_pwrup_mode(void)
{
	unsigned char rx_data;
	unsigned char pwrup_mode;
	put_CR();
	put_string("Setup power up mode:");	put_CR();
	put_string("0. Not change!");	put_CR();
	put_string("1. Power up on, SW off-SW on!");	put_CR();
	put_string("2. Power switch on/off!");	put_CR();
	put_string("Pls input 0,1,2>");

	rx_data = (uart_getchar());
	uart_putchar(rx_data);	put_CR();

	pwrup_mode = EEPROM_read(PWRUP_MODE_ADDR);

	if (rx_data == '0')
	{
		put_string("Haven't change!");	put_CR();
		return 1;
	}
	else if (rx_data == '1')
	{
		EEPROM_write(PWRUP_STS_ADDR,PWRUP_ON);
		delay_20mS();
		delay_20mS();
		EEPROM_write(PWRUP_MODE_ADDR,PWR_UP_SW_ON);
		delay_20mS();
		delay_20mS();

		put_string("Power up on, SW off-SW on!!");	put_CR();
		return 1;
	}
	else if (rx_data == '2')
	{
		EEPROM_write(PWRUP_STS_ADDR,PWRSW_ON);
		delay_20mS();
		delay_20mS();
		EEPROM_write(PWRUP_MODE_ADDR,PWR_SW_SW_ON);
		delay_20mS();
		delay_20mS();

		put_string("Power switch on/off!");	put_CR();
		return 1;
	}
	else
	{
		put_CR();
		put_string("Input is invalid, pls input 0,1,2!");	put_CR();
		return 0;
	}
}



void power_mode_check(void)
{
    unsigned char pwrup_mode;  //读取eeprom开机模式
  
   /*检测设置开机模式*/
	 pwrup_mode = EEPROM_read(PWRUP_MODE_ADDR);  //read up mode

	 if((pwrup_mode != PWR_SW_SW_ON) && (pwrup_mode != PWR_UP_SW_ON))
	 {
		EEPROM_write(PWRUP_MODE_ADDR,PWR_UP_SW_ON); //如果没有值直接写入来电自起模式
		delay_20mS();
		delay_20mS(); 
	 }
	 
	//display power up on mode
	pwrup_mode = EEPROM_read(PWRUP_MODE_ADDR);

	if (pwrup_mode == PWR_UP_SW_ON)  //如果是来电自起模式
	{
		pwrup_sts = EEPROM_read(PWRUP_STS_ADDR);
		if (pwrup_sts != PWRUP_ON)
		{
			EEPROM_write(PWRUP_STS_ADDR,PWRUP_ON);
			delay_20mS();
			delay_20mS();
		}
		put_string("Power up on, SW on/off mode!");
        put_CR();
	}
	else if (pwrup_mode == PWR_SW_SW_ON)
	{
		pwrup_sts = EEPROM_read(PWRUP_STS_ADDR);
		if (pwrup_sts != PWRSW_ON)
		{
			EEPROM_write(PWRUP_STS_ADDR,PWRSW_ON);
			delay_20mS();
			delay_20mS();
		}
		put_string("SW on/off mode!");
		put_CR();
	}  
}

void kvm_mode_check()
{
    unsigned char kvm_mode;  //读取KVM显示模式

   /*检测设置开机模式*/
	 kvm_mode = EEPROM_read(KVM_STS_ADDR);  //read kvm mode

	 if((kvm_mode != ACTION_MODEM) && (kvm_mode != DISP_MODEM))
	 {
		EEPROM_write(KVM_STS_ADDR,ACTION_MODEM); //如果没有值直接写入来电自起模式
		delay_20mS();
		delay_20mS(); 
	 }
	 
	kvm_mode = EEPROM_read(KVM_STS_ADDR);

	if (kvm_mode == ACTION_MODEM)  //如果是action一体机
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
	else if (kvm_mode == DISP_MODEM)  //显示器模式
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
}
//主程序
void main(void)
{   
	init_devices(); //初始化所有状态
	power_mode_check();
    kvm_mode_check();
	   
   /* while(1) 
    { 
	    delay_1s();
		display_EEPROM();
    }*/
	
	while (1)
	{
		pwrup_sts = EEPROM_read(PWRUP_STS_ADDR);

		if (pwrup_sts == PWRUP_ON) //power up on  lai dian zi qi
		{
			//PWR_ON();
			put_string("Power up on"); 
            put_CR();
		}
		else if (pwrup_sts == PWRSW_ON) //power switch on  anjian kaji
		{
			while (1)
			{
				while (((PIND & 0x04) == 0x04) && (rx_int_flag == 0));  //读取按键状态 PD2 
				if (rx_int_flag == 1)
				{ 
				   	rx_int_flag = 0;
                    setup();  //设置启动模式，设置后手动断电重起生效
				}
				delay_1s();
				
				if ((PIND & 0x04) == 0x04)
				{
					//PWR_ON();  //开机
					EEPROM_write(PWRUP_STS_ADDR,PWRUP_ON);
					delay_20mS();
					delay_20mS();
					put_string("Power switch on!"); put_CR();
					break;
				}
			}
		}
		
		if((rx_int_flag == 1) && (uart_getchar()== 'P'))  //当收到'P''O'两个字符，执行软关机动作
		{   rx_int_flag == 0;
		    if((rx_int_flag == 1) && (uart_getchar()== 'O'))
			{
			    rx_int_flag == 0;
			    //执行关机动作
			}
		}
		if((rx_int_flag == 1) && (uart_getchar()== 'S'))  //当收到'S''U'两个字符，设置开机模式为：上电自启
		{   rx_int_flag == 0;
		    if((rx_int_flag == 1) && (uart_getchar()== 'U'))
			{
			    rx_int_flag == 0;
			    EEPROM_write(PWRUP_STS_ADDR,PWRUP_ON);
				delay_20mS();
				delay_20mS();
				EEPROM_write(PWRUP_MODE_ADDR,PWR_UP_SW_ON);
				delay_20mS();
				delay_20mS();
			}
		}
		
		if((rx_int_flag == 1) && (uart_getchar()== 'B'))  //当收到'B''U'两个字符，设置开机模式为：按键开机
		{   rx_int_flag == 0;
		    if((rx_int_flag == 1) && (uart_getchar()== 'U'))
			{
			    rx_int_flag == 0;
			    EEPROM_write(PWRUP_STS_ADDR,PWRSW_ON);
				delay_20mS();
				delay_20mS();
				EEPROM_write(PWRUP_MODE_ADDR,PWR_SW_SW_ON);
				delay_20mS();
				delay_20mS();
			}
		}
		
	} 
}
