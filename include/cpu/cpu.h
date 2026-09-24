#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include <stdbool.h>

typedef enum { Z = 7, N = 6, H = 5, C = 4 } Flag;

uint8_t cpu_step(Memory* memory, Register* registers);
void cpu_set_interrupt_master_enable(bool value);
void cpu_set_interrupt_enable_pending(void);
bool cpu_interrupt_master_pending(void);
void cpu_clear_interrupt_enable_pending(void);
bool cpu_is_halted(void);
bool cpu_interrupt_master_enable(void);
void cpu_reset_state(void);

uint8_t fetch_byte(Memory* memory, uint16_t* address);
memory16 fetch_two_bytes(Memory* memory, uint16_t* address);
bool get_register_flag(Register* registers, Flag flag);
void set_register_flag(Register* registers, Flag flag, bool value);

#endif