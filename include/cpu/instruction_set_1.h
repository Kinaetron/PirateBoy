#ifndef INSTRUCTION_SET_1
#define INSTRUCTION_SET_1

#include "memory.h"
#include <stdbool.h>

void cpu_reset_state(void);
bool cpu_is_halted(void);
bool cpu_interrupt_master_enable(void);
bool cpu_interrupt_master_pending(void);
uint8_t instruction_set_1_step(CPU_Memory* memory, uint8_t opcode);

#endif