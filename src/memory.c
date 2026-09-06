#include "timer.h"
#include "memory.h"

uint8_t memory_read(CPU_Memory* memory, uint16_t address)
{
	if (address >= ROM_START && address <= ROM_END) {
		return memory->rom[address];
	}
	else if (address >= WRAM_START && address <= WRAM_END) {
		return memory->wram[address - WRAM_START];
	}
	else if (address >= ECHO_RAM_START && address <= ECHO_RAM_END) {
		return memory->wram[address - ECHO_RAM_START];
	}
	else if (address == INTERRUPT_FLAG_ADDR) {
		return memory->interrupt_flag;
	}
	else if (address >= IO_START && address <= IO_END) {
		return memory->input_output[address - IO_START];
	}
	else if (address >= HRAM_START && address <= HRAM_END) {
		return memory->hram[address - HRAM_START];
	}
	else if (address == INTERRUPT_ENABLE_ADDR) {
		return memory->interrupt_enable;
	}

	return MMU_UNMAPPED_READ_VALUE;
}

void memory_write(CPU_Memory* memory, uint16_t address, uint8_t data)
{
	if (address >= ROM_START && address <= ROM_END) {
		return;
	}
	else if (address >= WRAM_START && address <= WRAM_END) {
		memory->wram[address - WRAM_START] = data;
	}
	else if (address >= ECHO_RAM_START && address <= ECHO_RAM_END) {
		memory->wram[address - ECHO_RAM_START] = data;
	}
	else if (address == DIVIDER_REGISTER) 
	{
		memory->input_output[DIVIDER_REGISTER - IO_START] = 0;
		timer_reset_divider();
	}
	else if (address == INTERRUPT_FLAG_ADDR) {
		memory->interrupt_flag = data;
	}
	else if (address >= IO_START && address <= IO_END) {
		memory->input_output[address - IO_START] = data;
	}
	else if (address >= HRAM_START && address <= HRAM_END) {
		memory->hram[address - HRAM_START] = data;
	}
	else if (address == INTERRUPT_ENABLE_ADDR) {
		memory->interrupt_enable = data;
	}
	else
	{
#ifdef DEBUG
		fprintf(stderr, "Unmapped write: addr=0x%04X data=0x%02X\n", address, data);
#endif
	}
}

void memory_divider_register_incrementer(CPU_Memory* memory) {
	memory->input_output[DIVIDER_REGISTER - IO_START]++;
}