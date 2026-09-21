#include "memory.h"
#include <stdbool.h>

bool is_pending(Memory* memory) {
	return (memory->flat[INTERRUPT_ENABLE_ADDR] & memory->flat[INTERRUPT_FLAG_ADDR] & 0x1F) == 0;
}

uint8_t memory_read(Memory* memory, uint16_t address) {
	return memory->flat[address];
}

void memory_write(Memory* memory, uint16_t address, uint8_t data) {
	memory->flat[address] = data;
}

void memory_divider_register_incrementer(Memory* memory) {}

void set_if_interrupt(Memory* memory, Interrupt_Flag flag, bool value)
{
	if (value) {
		memory->flat[INTERRUPT_FLAG_ADDR] |= (1 << flag);
	}
	else {
		memory->flat[INTERRUPT_FLAG_ADDR] &= ~(1 << flag);
	}
}