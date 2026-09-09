#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include <stdbool.h>

typedef enum { Z = 7, N = 6, H = 5, C = 4 } Flag;

void cpu_reset_state(void);
bool cpu_is_halted(void);
bool cpu_interrupt_master_enable(void);
bool cpu_interrupt_master_pending(void);
uint8_t cpu_step(CPU_Memory* memory);
void cpu_set_interrupt_master_enable(bool value);
bool is_pending(CPU_Memory* memory);

#endif