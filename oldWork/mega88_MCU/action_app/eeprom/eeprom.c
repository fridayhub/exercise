#include "../act_include.h"

void EEPROM_write( unsigned int address, unsigned char data)
{
    unsigned char cSREG;

	/* Wait for completion of previous write */
    while (EECR & (1 << EEPE))  //EECR The EEPROM Control Register , EEPE 
		; 
	
	/* Set up address and Data Registers */
	EEAR = address;  //EEAR The EEPROM Address Register
	EEDR = data;    //EEDR The EEPROM Data Register
	
	cSREG = SREG;
	CLI();	
	EECR |= BIT(EEMWE);
	EECR |= BIT(EEWE);
	while (EECR & EEPE);
	SREG = cSREG;
}

unsigned char EEPROM_read(unsigned int address)
{
	/* Wait for completion of previous write */
    while (EECR & (1 << EEPE))
		;
	
	/* Set up address register */
	EEAR = address;
	
	/* Start eeprom read by writing EERE */
    EECR |= (1 << EEPE);
	
	/* Return data from Data Register */
    return (EEDR);
}

//读出并显示数据
void display_EEPROM(void)
{
	unsigned char i = 0;
	for (i = 0; i < 32; i++)
	{
		put_char_to_ascii(EEPROM_read(32));
		//put_char_to_ascii(EEPROM_read(33));
		//put_char_to_ascii(EEPROM_read(34));
		if (((i+1) % 4) == 0)
			put_CR();
		else
			uart_putchar(' ');
	}
}