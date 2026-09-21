#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include <stdbool.h>

typedef enum { Z = 7, N = 6, H = 5, C = 4 } Flag;


uint8_t cpu_step(Memory* memory, Register* registers);
void cpu_set_interrupt_master_enable(bool value);
bool is_pending(Memory* memory);

uint8_t fetch_byte(Memory* memory, uint16_t* address);
memory16 fetch_two_bytes(Memory* memory, uint16_t* address);
bool get_register_flag(Register* registers, Flag flag);
void set_register_flag(Register* registers, Flag flag, bool value);

#endif