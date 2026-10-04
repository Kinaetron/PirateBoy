#include <stdio.h>
#include <stdlib.h>

#include "timer.h"
#include "memory.h"

#define TITLE_OFFSET 0x0134
#define MANUFACTURER_OFFSET 0x013F
#define MANUFACTURER_LENGTH 4
#define OLD_LICENSEE_OFFSET 0x014B
#define NEW_LICENSEE_OFFSET 0x0144
#define CARTRIDGE_TYPE_OFFSET 0x0147
#define ROM_SIZE_OFFSET 0x0148
#define RAM_SIZE_OFFSET 0x0149
#define HEADER_MINIMUM_SIZE 0x0150
#define TITLE_LENGTH 11
#define CGB_FLAG_OFFSET 0x0143
#define SGB_FLAG_OFFSET 0x0146
#define HEADER_CHECKSUM_OFFSET 0x014D
#define CHECKSUM_RANGE_START 0x0134
#define CHECKSUM_RANGE_END 0x014C

#define JOYPAD_REGISTER 0xFF00
#define SERIAL_DATA     0xFF01
#define SERIAL_CONTROL  0xFF02
#define DMA_REGISTER    0xFF46

bool is_pending(Memory* memory)  {
	return (memory->flat[INTERRUPT_ENABLE_ADDR] & memory->flat[INTERRUPT_FLAG_ADDR] & 0x1F) != 0;
}

static bool parse_rom_header(Rom* rom)
{
	if (rom->size < HEADER_MINIMUM_SIZE) {
		return false;
	}

	uint8_t checksum = 0;
	for (size_t i = CHECKSUM_RANGE_START; i <= CHECKSUM_RANGE_END; i++) {
		checksum = checksum - rom->data[i] - 1;
	}

	rom->header_checksum = rom->data[HEADER_CHECKSUM_OFFSET];
	if (checksum != rom->header_checksum) {
		return false;
	}

	memcpy(rom->title, &rom->data[TITLE_OFFSET], TITLE_LENGTH);
	rom->title[TITLE_LENGTH] = '\0';

	memcpy(rom->manufacturer_code, &rom->data[MANUFACTURER_OFFSET], MANUFACTURER_LENGTH);
	rom->manufacturer_code[MANUFACTURER_LENGTH] = '\0';

	rom->cgb_flag = rom->data[CGB_FLAG_OFFSET];
	rom->sgb_flag = rom->data[SGB_FLAG_OFFSET];

	uint8_t old_licensee = rom->data[OLD_LICENSEE_OFFSET];

	if (old_licensee == 0x33)
	{
		rom->licensee_code[0] = rom->data[NEW_LICENSEE_OFFSET];
		rom->licensee_code[1] = rom->data[NEW_LICENSEE_OFFSET + 1];
	}
	else
	{
		rom->licensee_code[0] = old_licensee;
		rom->licensee_code[1] = 0;
	}

	rom->cartridge_type = (Cartridge_Type)rom->data[CARTRIDGE_TYPE_OFFSET];
	rom->rom_size = rom->data[ROM_SIZE_OFFSET];
	rom->ram_size = rom->data[RAM_SIZE_OFFSET];

	return true;
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

	Rom* rom = calloc(1, sizeof(Rom));
	if (rom == NULL)
	{
		fclose(file);
		return false;
	}

	rom->data = calloc(1, (size_t)file_size);
	if (rom->data == NULL)
	{
		free(rom);
		fclose(file);
		return false;
	}

	size_t read = fread(rom->data, 1, (size_t)file_size, file);
	fclose(file);

	if (read != (size_t)file_size)
	{
		free(rom->data);
		free(rom);
		return false;
	}

	rom->size = (size_t)file_size;

	if (!parse_rom_header(rom))
	{
		free(rom->data);
		free(rom);
		return false;
	}

	memory->rom = rom;

	size_t copy_size = (rom->size < 0x8000) ? rom->size : 0x8000;
	memcpy(memory->flat, rom->data, copy_size);

	return true;
}

void unload_rom(Memory* memory)
{
	if (memory->rom == NULL) {
		return;
	}

	free(memory->rom->data);
	free(memory->rom);
	memory->rom = NULL;
}

uint8_t memory_read(Memory* memory, uint16_t address)
{
	if (address >= ECHO_RAM_START && address <= ECHO_RAM_END) {
		return memory->flat[address - (ECHO_RAM_START - WRAM_START)];
	}

	return memory->flat[address];
}

void write_byte(Memory* memory, uint16_t* address, uint8_t data)
{
	*address = *address - 1;
	memory_write(memory, *address, data);
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
	else if (address >= ECHO_RAM_START && address <= ECHO_RAM_END) 
	{
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

void memory_post_boot(Memory* memory)
{
	memory->flat[JOYPAD_REGISTER] = 0xCF;
	memory->flat[SERIAL_DATA] = 0x00;
	memory->flat[SERIAL_CONTROL] = 0x7E;
	memory->flat[INTERRUPT_FLAG_ADDR] = 0xE1;
	memory->flat[DMA_REGISTER] = 0xFF;
	memory->flat[INTERRUPT_ENABLE_ADDR] = 0x00;
}