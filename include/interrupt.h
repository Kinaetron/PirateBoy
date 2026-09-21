#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "memory.h"

uint8_t handle_interrupts(Memory* memory, Register* registers);

#endif