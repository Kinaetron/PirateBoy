#ifndef CPU_H
#define CPU_H

#include "memory.h"

typedef enum { Z = 7, N = 6, H = 5, C = 4 } Flag;
uint8_t cpu_step(CPU_Memory* memory);

typedef union
{
	uint16_t value;
	struct
	{
		uint8_t low;
		uint8_t high;
	};
} memory16;

#endif