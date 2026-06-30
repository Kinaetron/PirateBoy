#include "cpu.h"
#include "memory.h"
#include <stdbool.h>

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

static void memory_write(CPU_Memory* memory, uint16_t address, uint8_t data)
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

static void set_register_flag(CPU_Memory* memory, Flag flag, bool value)
{
	if (value) {
		memory->af.low |= (1 << flag);
	}
	else {
		memory->af.low &= ~(1 << flag);
	}

	memory->af.low &= 0xF0;
}

static uint8_t fetch_byte(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->program_counter);
	memory->program_counter++;
	return value;
}

static uint8_t opcode_0x01(CPU_Memory* memory)
{
	uint8_t low = fetch_byte(memory);
	uint8_t high = fetch_byte(memory);

	memory->bc.low = low;
	memory->bc.high = high;

	return 12;
}

static uint8_t opcode_0x02(CPU_Memory* memory)
{
	uint8_t a_register = memory->af.high;
	memory_write(memory, memory->bc.value, a_register);

	return 8;
}

static uint8_t opcode_0x03(CPU_Memory* memory) 
{
	memory->bc.value++;

	return 8;
}

static uint8_t opcode_0x04(CPU_Memory* memory)
{
	set_register_flag(memory, H, (memory->bc.high & 0x0F) == 0x0F);
	set_register_flag(memory, N, false);

	memory->bc.high++;

	set_register_flag(memory, Z, memory->bc.high == 0x00);

	return 4;
}

static uint8_t opcode_0x05(CPU_Memory* memory)
{
	set_register_flag(memory, H, (memory->bc.high & 0x0F) == 0x00);
	set_register_flag(memory, N, true);

	memory->bc.high--;

	set_register_flag(memory, Z, memory->bc.high == 0x00);

	return 4;
}

uint8_t cpu_step(CPU_Memory* memory)
{
	uint8_t opcode = fetch_byte(memory);
	uint8_t cycles = 4;

	switch (opcode)
	{
		case 0x00:
			break;
		case 0x01:
			cycles = opcode_0x01(memory);
			break;
		case 0x02:
			cycles = opcode_0x02(memory);
			break;
		case 0x03:
			cycles = opcode_0x03(memory);
			break;
		case 0x04:
			cycles = opcode_0x04(memory);
			break;
		case 0x05:
			cycles = opcode_0x05(memory);
		default:
			break;
	}

	return cycles;
}