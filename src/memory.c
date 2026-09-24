#include "timer.h"
#include "memory.h"

bool is_pending(Memory* memory)  {
	return (memory->flat[INTERRUPT_ENABLE_ADDR] & memory->flat[INTERRUPT_FLAG_ADDR] & 0x1F) == 0;
}

uint8_t memory_read(Memory* memory, uint16_t address)
{
	if (address >= ECHO_RAM_START && address <= ECHO_RAM_END) {
		return memory->flat[address - (ECHO_RAM_START - WRAM_START)];
	}

	return memory->flat[address];
}

void memory_write(Memory* memory, uint16_t address, uint8_t data)
{
	if (address >= ROM_START && address <= ROM_END) {
		return;
	}
	else if (address >= VRAM_START && address <= VRAM_END)
	{
		memory->flat[address] = data;
		memory->vram_dirty = true;
		return;
	}
	else if (address >= ECHO_RAM_START && address <= ECHO_RAM_END) {
		memory->flat[address - (ECHO_RAM_START - WRAM_START)] = data;
		return;
	}
	else if (address == DIVIDER_REGISTER) {
		memory->flat[address] = 0;
		timer_reset_divider();
		return;
	}

	memory->flat[address] = data;
}

void memory_divider_register_incrementer(Memory* memory) {
	memory->flat[DIVIDER_REGISTER]++;
}

void set_if_interrupt(Memory* memory, Interrupt_Flag flag, bool value)
{
	if (value) {
		memory->flat[INTERRUPT_FLAG_ADDR] |= (1 << flag);
	}
	else {
		memory->flat[INTERRUPT_FLAG_ADDR] &= ~(1 << flag);
	}
}
