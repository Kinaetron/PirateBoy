#ifndef TIMER_H
#define TIMER_H

#include "memory.h"

#define DIVIDER_REGISTER	0XFF04
#define TIMER_COUNTER		0XFF05
#define TIMER_MODULO		0XFF06
#define TIMER_CONTROL		0XFF07

#define DIVIDER_INCREMENT	256

void timer_reset_divider(void);
void timer_step(Memory* memory, uint8_t cycles);
void timer_post_boot(Memory* memory);

#endif