#ifndef EERPOM_H
#define EERPOM_H

void EEPROM_write( unsigned int address, unsigned char data);
unsigned char EEPROM_read(unsigned int address);
void display_EEPROM(void);

#endif