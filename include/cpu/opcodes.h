#ifndef INSTRUCTION_SET_1
#define INSTRUCTION_SET_1

#include "memory.h"
#include <stdbool.h>

void cpu_reset_state(void);
bool cpu_is_halted(void);
bool cpu_interrupt_master_enable(void);
bool cpu_interrupt_master_pending(void);
uint8_t opcode_step(Memory* memory, Register* registers, uint8_t opcode);
bool get_ie_interrupt(Memory* memory, Interrupt_Flag flag);
bool get_if_interrupt(Memory* memory, Interrupt_Flag flag);
void write_byte(Memory* memory, uint16_t* address, uint8_t data);

#endif