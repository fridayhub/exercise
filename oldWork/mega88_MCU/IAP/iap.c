/*****************************************************
 采用串行接口实现Boot_load应用的实例
 Compiler: ICC-AVR 7.22
 Target: Mega88
 Crystal: 1Mhz
 Used: T/C0,USART0
 *****************************************************/
#include <iom88v.h>
#include <macros.h>
#include <AVRdef.h>      //中断函数头文件

#define SPM_PAGESIZE 64 //M88的一个Flash页为64字节(32字)
#define BAUD 9600     //波特率采用9600bps


#define BAUD_H 0x00
#define BAUD_L 0x0C  
		
//定义Xmoden控制字符
#define XMODEM_NUL 0x00
#define XMODEM_SOH 0x01
#define XMODEM_STX 0x02
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_CAN 0x18
#define XMODEM_EOF 0x1A
#define XMODEM_RECIEVING_WAIT_CHAR 'C'
//定义全局变量
unsigned char startupString[]="Types 'd' download, Others run app.\n\r\0";
char data[128];
long address = 0;

//等待一个Flash页的写完成
 void wait_page_rw_ok(void)
{ 
  while(SPMCSR & (1<<RWWSB)) {   
        while(SPMCSR & (1<<SPMEN));   
        SPMCSR = 0x11;   
        asm("spm ");   
    }   
}

//擦除(code=0x03)和写入(code=0x05)一个Flash页
void boot_page_ew(long p_address,char code)
{ 
    asm("movw r30,  r16"); //将页地址放入Z寄存器和RAMPZ的Bit0中  
	//same as asm("mov r30,r16\n""mov r31,r17\n""out 0x3b,r18\n"); 
    SPMCSR = code;   
    asm("spm");   
   
    wait_page_rw_ok();
}
 
//填充Flash缓冲页中的一个字
void boot_page_fill(unsigned int address,int rdata)
{ 
    asm("movw r30,r16");   
    asm("movw r0,r18"); 
	//same as asm("mov r30,r16\n""mov r31,r17\n" //Z寄存器中为填冲页内地址 
	//"mov r0,r18\n""mov r1,r19\n");  //R0R1中为一个指令字   
    SPMCSR = 0x01;   
    asm("spm");           
}
 
//更新一个Flash页的完整处理 因为一个页64Byte 所以需要写两次
void write_one_page(void)
{ 
      int i; 
      boot_page_ew(address, 0x03); //擦除一个Flash页 
      wait_page_rw_ok(); //等待擦除完成 
      for(i = 0;i < SPM_PAGESIZE; i += 2) //将数据填入Flash缓冲页中 
      { 
           boot_page_fill(i, (data[i] + (data[i+1]<<8))); 
      } 
      boot_page_ew(address,0x05); //将缓冲页数据写入一个Flash页 
      wait_page_rw_ok(); //等待写入完成
	  address += SPM_PAGESIZE; //Flash页加1
	  
	 boot_page_ew(address, 0x03); //擦除一个Flash页 
	  wait_page_rw_ok(); //等待擦除完成 
	  for (i = SPM_PAGESIZE; i < 128; i += 2) //将数据填入Flash缓冲页中 
	  {
		  boot_page_fill(i, (data[i] + (data[i + 1] << 8)));
	  }
	  boot_page_ew(address, 0x05); //将缓冲页数据写入一个Flash页 
	  wait_page_rw_ok(); //等待写入完成 	
	  address += SPM_PAGESIZE;  //Flash页加1 
}
 
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
 
//计算CRC
int calcrc(char *ptr, int count)
{ 
  int crc = 0; 
  char i; 
  while (--count >= 0) 
  { 
    crc = crc ^ (int) *ptr++ << 8; 
    i = 8; 
    do 
     { 
      if (crc & 0x8000) 
       crc = crc << 1 ^ 0x1021; 
      else 
       crc = crc << 1; 
       } while(--i); 
     } 
  return (crc);
 }
 
//退出Bootloader程序，从0x0000处执行应用程序
 void quit(void)
 { 
      uart_putchar('O');uart_putchar('K'); 
      uart_putchar(0x0d);uart_putchar(0x0a);
      while(!(UCSR0A & 0x20)); //等待结束提示信息回送完成 
      MCUCR = 0x01; 
      MCUCR = 0x00; //将中断向量表迁移到应用程序区头部 
      //RAMPZ = 0x00; //RAMPZ清零初始化 mega88没有RAMPZ寄存器
     // asm("jmp 0x0000 "); //跳转到Flash的0x0000处，执行用户的应用程序
	 asm("LDI R30,0X00\n"   //LDI 装入立即数
     "LDI R31,0X00\n"     //z寄存器初始化
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
	  
	 //向PC机发送开始提示信息  
	 p = startupString;
	 while(*p != '\0') 
     { 
        uart_putchar(*p++); 
     } 
 
     //3秒种等待PC下发"d"，否则退出Bootloader程序，从0x0000处执行应用程序 
     while(1) 
      { 
         if(uart_getchar()== 'd') break; 

		 if(onesecond == 1)
	     {
	       onesecond = 0;
	       my_count++;
	       if(my_count == 3)  //延时3s
	        {
		     my_count = 0;
	         quit();
	        }
	      }
      } 
      //每秒向PC机发送一个控制字符"C"，等待控制字〈soh〉 
      while(uart_getchar() != XMODEM_SOH) //receive the start of Xmodem 
      {  		
		if(onesecond == 1)
	     {
	       onesecond = 0;
	       uart_putchar(XMODEM_RECIEVING_WAIT_CHAR); //send a "C"
         }
	  } 
      //开始接收数据块 
      do 
      { 
            if ((packNO == uart_waitchar()) && (packNO ==(~uart_waitchar()))) 
              { //核对数据块编号正确 
                  for(i=0; i < 128; i++) //接收128个字节数据 
                  { 
					 data[bufferPoint] = uart_waitchar();
                     bufferPoint++; 
                  } 
                  crc = (uart_waitchar()<<8); 
                  crc += uart_waitchar(); //接收2个字节的CRC效验字

                  if(calcrc(data, 128) == crc) //CRC校验验证 
                  { //正确接收128个字节数据 
					  if((address < 0xC00) && (bufferPoint >= 128)) //0xC00 以上为 bootloader区  //正确接受128个字节的数据 
                       {  
						  write_one_page(); //收到128字节写入一页Flash中 
						  bufferPoint = 0;
                       }
                        uart_putchar(XMODEM_ACK); //正确收到一个数据块 
                        packNO++; //数据块编号加1 
                  } 
                  else 
                  { 
                        uart_putchar(XMODEM_NAK); //要求重发数据块 
                  } 
            } 
            else 
            { 
                 uart_putchar(XMODEM_NAK); //要求重发数据块 
            } 
      }while(uart_waitchar() != XMODEM_EOT); //循环接收，直到全部发完 
      uart_putchar(XMODEM_ACK); //通知PC机全部收到 
	  
      if(bufferPoint)
	  {
          write_one_page(); //把剩余的数据写入Flash中 
	  }
      quit(); //退出Bootloader程序，从0x0000处执行应用程序  
 }
