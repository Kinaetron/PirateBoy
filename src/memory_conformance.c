#include "memory.h"
#include <stdbool.h>

bool is_pending(CPU_Memory* memory) {
	return (memory->flat[INTERRUPT_ENABLE_ADDR] & memory->flat[INTERRUPT_FLAG_ADDR] & 0x1F) == 0;
}

uint8_t memory_read(CPU_Memory* memory, uint16_t address)
{
	if (address >= ECHO_RAM_START && address <= ECHO_RAM_END) {
		return memory->flat[address - (ECHO_RAM_START - WRAM_START)];
	}
	return memory->flat[address];
}

void memory_write(CPU_Memory* memory, uint16_t address, uint8_t data)
{
	if (address >= ECHO_RAM_START && address <= ECHO_RAM_END) {
		memory->flat[address - (ECHO_RAM_START - WRAM_START)] = data;
		return;
	}

	memory->flat[address] = data;
}

void memory_divider_register_incrementer(CPU_Memory* memory) { }

void set_if_interrupt(CPU_Memory* memory, Interrupt_Flag flag, bool value)
{
	if (value) {
		memory->flat[INTERRUPT_FLAG_ADDR] |= (1 << flag);
	}
	else {
		memory->flat[INTERRUPT_FLAG_ADDR] &= ~(1 << flag);
	}
}