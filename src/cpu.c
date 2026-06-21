#include "cpu.h"
#include "memory.h"

static uint8_t memory_read(CPU_Memory* memory, uint16_t address)
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

uint8_t cpu_fetch_byte(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->program_counter);
	memory->program_counter++;
	return value;
}

void cpu_step(CPU_Memory* memory)
{
	uint8_t opcode = cpu_fetch_byte(memory);

	switch (opcode)
	{
		case 0x00:
			break;

		default:
			break;
	}
}