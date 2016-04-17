#ifndef INIT_H
#define INIT_H

#define SPM_PAGESIZE 64  //M88的一个Flash页为64字节(32字)

//KVM 相关定义  
#define KVM_STS_ADDR    0x22  //KVM状态地址
#define DISP_MODEM      0x01  //显示器模式
#define ACTION_MODEM    0x00  //action 一体机模式

//EEPROM ADDRESS
#define PWRUP_STS_ADDR  0x20  //状态地址
#define PWRUP_MODE_ADDR 0x21  //模式地址  

#define PWRUP_ON        0x5A  //来电自启状态
#define PWRSW_ON        0x6B  //按键启动状态 
#define PWR_UP_SW_ON    0x7C   //来电自启模式 
#define PWR_SW_SW_ON    0x8D  //按键启动模式


#define PC4_DIS         PORTC &= ~BIT(4)
#define PC0_DIS         PORTC &= ~BIT(0)
#define PC0_EN          PORTC |= BIT(0)
#define PB6_EN          PORTB |= BIT(6)

//高电平有效--指示当前LCD显示的内容为CPU画面
#define KVM_LED1_on     PORTB |= BIT(7)
#define KVM_LED1_off    PORTB &= ~BIT(7)

//高电平有效--指示当前LCD显示的内容为PC或外置一侧视频信号
#define KVM_LED2_on     PORTB |= BIT(4) 
#define KVM_LED2_off    PORTB &= ~BIT(4) 

//低电平有效--将USB键盘+鼠标切换至CPU一侧
#define KVM_USB0_on     PORTD |= BIT(5)
#define KVM_USB0_off    PORTD &= ~BIT(5)

//低电平有效--将USB键盘+鼠标切换至PC或外置一侧
#define KVM_USB1_on     PORTD |= BIT(6)
#define KVM_USB1_off    PORTD &= ~BIT(6)

//低电平有效--使得Scale输出至LCD显示内容为CPU视频信号
#define SCALE_SW0_on    PORTC |= BIT(2)
#define SCALE_SW0_off   PORTC &= ~BIT(2)

//高电平有效--使得Scale输出至LCD显示内容为PC或外置信号
#define SCALE_SW1_on    PORTC |= BIT(3)
#define SCALE_SW1_off   PORTC &= ~BIT(3)

void init_devices(void);

#endif