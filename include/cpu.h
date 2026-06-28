#ifndef CPU_H
#define CPU_H

#include "memory.h"

typedef enum { Z = 7, N = 6, H = 5, C = 4 } Flag;
void cpu_step(CPU_Memory* memory);

#endif