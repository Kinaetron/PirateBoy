#include <stdio.h>

#include "timer.h"
#include "memory.h"

bool is_pending(Memory* memory)  {
	return (memory->flat[INTERRUPT_ENABLE_ADDR] & memory->flat[INTERRUPT_FLAG_ADDR] & 0x1F) == 0;
}

static bool parse_rom_header(Rom* rom)
{

}

bool load_rom(Memory* memory, const char* path)
{
	FILE* file = fopen(path, "rb");

	if (file == NULL) {
		return false;
	}

	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (file_size <= 0)
	{
		fclose(file);
		return false;
	}

	Rom* rom = malloc(sizeof(Rom));
	if (rom == NULL)
	{
		fclose(file);
		return false;
	}

	rom->data = malloc((size_t)file_size);
	if (rom->data == NULL)
	{
		free(rom);
		fclose(file);
		return false;
	}
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
