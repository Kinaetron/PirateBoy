#include <stdlib.h>
#include "cartridge/mbc0.h"

typedef struct
{
	const uint8_t* rom;
	size_t rom_size;
} Mbc0State;

static uint8_t mbc0_read_rom(Mapper* mapper, uint16_t address)
{
	Mbc0State* state = (Mbc0State*)mapper->state;

	if (address < state->rom_size) {
		return state->rom[address];
	}

	return 0xFF;
}

static void mbc0_write_rom(Mapper* mapper, uint16_t address, uint8_t value) { }

static uint8_t mbc0_read_ram(Mapper* mapper, uint16_t address) {
	return 0xFF;
}

static void mbc0_write_ram(Mapper* mapper, uint16_t address, uint8_t value) { }

Mapper* create_mbc0(const uint8_t* rom, size_t rom_size)
{
	Mapper* mapper = calloc(1, sizeof(Mapper));
	if (mapper == NULL) {
		return NULL;
	}

	Mbc0State* state = calloc(1, sizeof(Mbc0State));
	if (state == NULL)
	{
		free(mapper);
		return NULL;
	}

	state->rom = rom;
	state->rom_size = rom_size;

	mapper->state = state;
	mapper->read_rom = mbc0_read_rom;
	mapper->write_rom = mbc0_write_rom;
	mapper->read_ram = mbc0_read_ram;
	mapper->write_ram = mbc0_write_ram;

	return mapper;
}