#include <stdlib.h>
#include <stdbool.h>
#include "cartridge/mbc1.h"

#define OPEN_BUS_VALUE		   0xFF

#define MBC1_RAM_ENABLE_END    0x1FFF
#define MBC1_BANK_LOW_END      0x3FFF
#define MBC1_BANK_HIGH_END     0x5FFF
#define MBC1_MODE_END          0x7FFF

#define MBC1_RAM_ENABLE_MASK   0x0F
#define MBC1_RAM_ENABLE_VALUE  0x0A
#define MBC1_BANK_LOW_MASK     0x1F
#define MBC1_BANK_HIGH_MASK    0x03
#define MBC1_MODE_MASK         0x01

#define ROM_BANK_SIZE        0x4000
#define ROM_BANK_OFFSET_MASK (ROM_BANK_SIZE - 1)
#define MBC1_BANK_HIGH_SHIFT 5

#define RAM_BANK_SIZE        0x2000
#define EXTERNAL_RAM_START	 0xA000

typedef struct
{
	const uint8_t* rom;
	size_t rom_size;

	uint8_t* ram;
	size_t ram_size;

	bool ram_enabled;
	uint8_t bank_low;
	uint8_t bank_high;
	bool mode;
} Mbc1State;

static uint8_t mbc1_read_rom(Mapper* mapper, uint16_t address)
{
	uint8_t bank;
	Mbc1State* state = (Mbc1State*)mapper->state;

	if (address < ROM_BANK_SIZE) {
		bank = state->mode ? (state->bank_high << MBC1_BANK_HIGH_SHIFT) : 0;
	}
	else {
		bank = (state->bank_high << MBC1_BANK_HIGH_SHIFT) | state->bank_low;
	}

	bank %= (state->rom_size / ROM_BANK_SIZE);

	return state->rom[(size_t)bank * ROM_BANK_SIZE + (address & ROM_BANK_OFFSET_MASK)];
}

static void mbc1_write_rom(Mapper* mapper, uint16_t address, uint8_t value)
{
	Mbc1State* state = (Mbc1State*)mapper->state;

	if (address <= MBC1_RAM_ENABLE_END) {
		state->ram_enabled = ((value & MBC1_RAM_ENABLE_MASK) == MBC1_RAM_ENABLE_VALUE);
	}
	else if (address <= MBC1_BANK_LOW_END)
	{
		state->bank_low = value & MBC1_BANK_LOW_MASK;

		if (state->bank_low == 0) {
			state->bank_low = 1;
		}
	}
	else if (address <= MBC1_BANK_HIGH_END) {
		state->bank_high = value & MBC1_BANK_HIGH_MASK;
	}
	else {
		state->mode = value & MBC1_MODE_MASK;
	}
}

static uint8_t mbc1_read_ram(Mapper* mapper, uint16_t address) 
{
	Mbc1State* state = (Mbc1State*)mapper->state;

	if (!state->ram_enabled || state->ram_size == 0) {
		return OPEN_BUS_VALUE;
	}
	
	uint8_t bank = state->mode ? state->bank_high : 0;
	size_t index = (size_t)bank * RAM_BANK_SIZE + (address - EXTERNAL_RAM_START);

	return state->ram[index % state->ram_size];
}

static void mbc1_write_ram(Mapper* mapper, uint16_t address, uint8_t value) 
{
	Mbc1State* state = (Mbc1State*)mapper->state;

	if (!state->ram_enabled || state->ram_size == 0) {
		return;
	}

	uint8_t bank = state->mode ? state->bank_high : 0;
	size_t index = (size_t)bank * RAM_BANK_SIZE + (address - EXTERNAL_RAM_START);

	state->ram[index % state->ram_size] = value;
}

static void mbc1_destroy(Mapper* mapper)
{
	Mbc1State* state = (Mbc1State*)mapper->state;

	free(state->ram);
	free(state);
}

Mapper* create_mbc1(const uint8_t* rom, size_t rom_size, size_t ram_size)
{
	Mapper* mapper = calloc(1, sizeof(Mapper));
	if (mapper == NULL) {
		return NULL;
	}

	Mbc1State* state = calloc(1, sizeof(Mbc1State));
	if (state == NULL)
	{
		free(mapper);
		return NULL;
	}

	if (ram_size > 0)
	{
		state->ram = calloc(1, ram_size);
		if (state->ram == NULL)
		{
			free(state);
			free(mapper);
			return NULL;
		}
	}

	state->rom = rom;
	state->rom_size = rom_size;
	state->ram_size = ram_size;
	state->bank_low = 1;

	mapper->state = state;
	mapper->read_rom = mbc1_read_rom;
	mapper->write_rom = mbc1_write_rom;
	mapper->read_ram = mbc1_read_ram;
	mapper->write_ram = mbc1_write_ram;
	mapper->destroy = mbc1_destroy;

	return mapper;
}