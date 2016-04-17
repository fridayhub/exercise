#ifndef TIMER_H
#define TIMER_H

void timer0_init(void);
void timer0_ovf_isr(void);
void delay_1s(void);
void delay_3s(void);
void delay_20mS(void);

#endif