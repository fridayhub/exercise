#include "../act_include.h"

void uart_init(void)
{
	 //初始化M88的USART0
	 UCSR0B = 0x00; //disable while setting baud rate 
	 UBRR0H = BAUD_H; 
	 UBRR0L = BAUD_L; //Set baud rate 
	 UCSR0B = 0x98; //Enable Receiver and Transmitter  enable RX interrupt
	 UCSR0C = 0x0E; //Set frame format: 8data, 2stop bit 
	 UCSR0A = 0x02;
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

 

/*
Description   : Convert Hex(0-F) to Character
Argument      : data -  Hex( 0x00'~0x0F)to convert into character (INPUT)
Note          :
*/
unsigned char hex_to_ascii(unsigned char data)
{
	if((data >= 0) && (data <= 9))
		return (data + '0');
	if((data >= 10) && (data <= 15))
		return  (data + 'A' - 10);
	return (data);
}

/*
Description   : Convert Character into Hex
Argument      : c_data - character( '0'~'F') to convert into Hex(INPUT)
Note          :
*/
unsigned char ascii_to_hex(unsigned char c_data)
{
	if( c_data >= '0' && c_data <= '9')
		return (c_data - '0');
	if( c_data >= 'A' && c_data <= 'F')
		return (10 + c_data -'A');
	if( c_data >= 'a' && c_data <= 'f')
		return (10 + c_data -'a');
	return (c_data);
}

/*
Description   : Output 1 Byte Hexadecimal digit to 2Byte ASCII character.  ex) 0x2E --> "2E"
Argument      : data - character to output(INPUT)
Return Value  :
Note          :  
*/
void put_char_to_ascii(unsigned char data)
{
	uart_putchar(hex_to_ascii(( data >> 4) & 0x0F));
	uart_putchar(hex_to_ascii(data & 0x0F));
}

/*
Description   : Output 2 Byte Integer to 4Byte ASCII character ex) 0x12FD --> "12FD"
Argument      : data - Integer to output(INPUT)
Return Value  :
Note          :  
*/
void put_int_to_ascii(unsigned int data)
{
	put_char_to_ascii(data / 0x100);
	put_char_to_ascii(data % 0x100);
}

/*
Description   : Output to Serial.
Argument      : Str - Character Stream to output (INPUT)
Return Value  :
Note          :
*/
void put_string(char *Str)
{
	unsigned int i = 0;
	
	while(Str[i] != 0)
	{
		uart_putchar(Str[i]);
		i++;
	}
}
 
 /*
Description   : Output 'CR' to Serial.
Argument      :
Return Value  :  
Note          :
*/

void put_CR(void)
{
	uart_putchar(0x0a);
	uart_putchar(0x0d);
}

void put_space(void)
{
 	 uart_putchar(0x20);
}