/*****************************************************
 ���ô��нӿ�ʵ��Boot_loadӦ�õ�ʵ��
 Compiler: ICC-AVR 7.22
 Target: Mega88
 Crystal: 1Mhz
 Used: T/C0,USART0
 *****************************************************/
#include <iom88v.h>
#include <macros.h>
#include <AVRdef.h>      //�жϺ���ͷ�ļ�

#define SPM_PAGESIZE 64 //M88��һ��FlashҳΪ64�ֽ�(32��)
#define BAUD 9600     //�����ʲ���9600bps


#define BAUD_H 0x00
#define BAUD_L 0x0C  
		
//����Xmoden�����ַ�
#define XMODEM_NUL 0x00
#define XMODEM_SOH 0x01
#define XMODEM_STX 0x02
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_CAN 0x18
#define XMODEM_EOF 0x1A
#define XMODEM_RECIEVING_WAIT_CHAR 'C'
//����ȫ�ֱ���
unsigned char startupString[]="Types 'd' download, Others run app.\n\r\0";
char data[128];
long address = 0;

//�ȴ�һ��Flashҳ��д���
 void wait_page_rw_ok(void)
{ 
  while(SPMCSR & (1<<RWWSB)) {   
        while(SPMCSR & (1<<SPMEN));   
        SPMCSR = 0x11;   
        asm("spm ");   
    }   
}

//����(code=0x03)��д��(code=0x05)һ��Flashҳ
void boot_page_ew(long p_address,char code)
{ 
    asm("movw r30,  r16"); //��ҳ��ַ����Z�Ĵ�����RAMPZ��Bit0��  
	//same as asm("mov r30,r16\n""mov r31,r17\n""out 0x3b,r18\n"); 
    SPMCSR = code;   
    asm("spm");   
   
    wait_page_rw_ok();
}
 
//���Flash����ҳ�е�һ����
void boot_page_fill(unsigned int address,int rdata)
{ 
    asm("movw r30,r16");   
    asm("movw r0,r18"); 
	//same as asm("mov r30,r16\n""mov r31,r17\n" //Z�Ĵ�����Ϊ���ҳ�ڵ�ַ 
	//"mov r0,r18\n""mov r1,r19\n");  //R0R1��Ϊһ��ָ����   
    SPMCSR = 0x01;   
    asm("spm");           
}
 
//����һ��Flashҳ���������� ��Ϊһ��ҳ64Byte ������Ҫд����
void write_one_page(void)
{ 
      int i; 
      boot_page_ew(address, 0x03); //����һ��Flashҳ 
      wait_page_rw_ok(); //�ȴ�������� 
      for(i = 0;i < SPM_PAGESIZE; i += 2) //����������Flash����ҳ�� 
      { 
           boot_page_fill(i, (data[i] + (data[i+1]<<8))); 
      } 
      boot_page_ew(address,0x05); //������ҳ����д��һ��Flashҳ 
      wait_page_rw_ok(); //�ȴ�д�����
	  address += SPM_PAGESIZE; //Flashҳ��1
	  
	 boot_page_ew(address, 0x03); //����һ��Flashҳ 
	  wait_page_rw_ok(); //�ȴ�������� 
	  for (i = SPM_PAGESIZE; i < 128; i += 2) //����������Flash����ҳ�� 
	  {
		  boot_page_fill(i, (data[i] + (data[i + 1] << 8)));
	  }
	  boot_page_ew(address, 0x05); //������ҳ����д��һ��Flashҳ 
	  wait_page_rw_ok(); //�ȴ�д����� 	
	  address += SPM_PAGESIZE;  //Flashҳ��1 
}
 
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
 
//����CRC
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
 
//�˳�Bootloader���򣬴�0x0000��ִ��Ӧ�ó���
 void quit(void)
 { 
      uart_putchar('O');uart_putchar('K'); 
      uart_putchar(0x0d);uart_putchar(0x0a);
      while(!(UCSR0A & 0x20)); //�ȴ�������ʾ��Ϣ������� 
      MCUCR = 0x01; 
      MCUCR = 0x00; //���ж�������Ǩ�Ƶ�Ӧ�ó�����ͷ�� 
      //RAMPZ = 0x00; //RAMPZ�����ʼ�� mega88û��RAMPZ�Ĵ���
     // asm("jmp 0x0000 "); //��ת��Flash��0x0000����ִ���û���Ӧ�ó���
	 asm("LDI R30,0X00\n"   //LDI װ��������
     "LDI R31,0X00\n"     //z�Ĵ�����ʼ��
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
	  
	 //��PC�����Ϳ�ʼ��ʾ��Ϣ  
	 p = startupString;
	 while(*p != '\0') 
     { 
        uart_putchar(*p++); 
     } 
 
     //3���ֵȴ�PC�·�"d"�������˳�Bootloader���򣬴�0x0000��ִ��Ӧ�ó��� 
     while(1) 
      { 
         if(uart_getchar()== 'd') break; 

		 if(onesecond == 1)
	     {
	       onesecond = 0;
	       my_count++;
	       if(my_count == 3)  //��ʱ3s
	        {
		     my_count = 0;
	         quit();
	        }
	      }
      } 
      //ÿ����PC������һ�������ַ�"C"���ȴ������֡�soh�� 
      while(uart_getchar() != XMODEM_SOH) //receive the start of Xmodem 
      {  		
		if(onesecond == 1)
	     {
	       onesecond = 0;
	       uart_putchar(XMODEM_RECIEVING_WAIT_CHAR); //send a "C"
         }
	  } 
      //��ʼ�������ݿ� 
      do 
      { 
            if ((packNO == uart_waitchar()) && (packNO ==(~uart_waitchar()))) 
              { //�˶����ݿ�����ȷ 
                  for(i=0; i < 128; i++) //����128���ֽ����� 
                  { 
					 data[bufferPoint] = uart_waitchar();
                     bufferPoint++; 
                  } 
                  crc = (uart_waitchar()<<8); 
                  crc += uart_waitchar(); //����2���ֽڵ�CRCЧ����

                  if(calcrc(data, 128) == crc) //CRCУ����֤ 
                  { //��ȷ����128���ֽ����� 
					  if((address < 0xC00) && (bufferPoint >= 128)) //0xC00 ����Ϊ bootloader��  //��ȷ����128���ֽڵ����� 
                       {  
						  write_one_page(); //�յ�128�ֽ�д��һҳFlash�� 
						  bufferPoint = 0;
                       }
                        uart_putchar(XMODEM_ACK); //��ȷ�յ�һ�����ݿ� 
                        packNO++; //���ݿ��ż�1 
                  } 
                  else 
                  { 
                        uart_putchar(XMODEM_NAK); //Ҫ���ط����ݿ� 
                  } 
            } 
            else 
            { 
                 uart_putchar(XMODEM_NAK); //Ҫ���ط����ݿ� 
            } 
      }while(uart_waitchar() != XMODEM_EOT); //ѭ�����գ�ֱ��ȫ������ 
      uart_putchar(XMODEM_ACK); //֪ͨPC��ȫ���յ� 
	  
      if(bufferPoint)
	  {
          write_one_page(); //��ʣ�������д��Flash�� 
	  }
      quit(); //�˳�Bootloader���򣬴�0x0000��ִ��Ӧ�ó���  
 }
