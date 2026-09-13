#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include <stdbool.h>

typedef enum { Z = 7, N = 6, H = 5, C = 4 } Flag;


uint8_t cpu_step(CPU_Memory* memory);
void cpu_set_interrupt_master_enable(bool value);
bool is_pending(CPU_Memory* memory);

uint8_t fetch_byte(CPU_Memory* memory, uint16_t* address);
memory16 fetch_two_bytes(CPU_Memory* memory, uint16_t* address);
bool get_register_flag(CPU_Memory* memory, Flag flag);
void set_register_flag(CPU_Memory* memory, Flag flag, bool value);

#endif