#ifndef INSTRUCTION_SET_1
#define INSTRUCTION_SET_1

#include "memory.h"
#include <stdbool.h>

bool cpu_consume_halt_bug(void);
uint8_t opcode_step(Memory* memory, Register* registers, uint8_t opcode);

#endif