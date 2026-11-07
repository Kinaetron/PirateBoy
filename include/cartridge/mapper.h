#ifndef MAPPER_H
#define MAPPER_H

#include <stdint.h>
#include "cartridge/cartridge_type.h"

typedef struct Mapper 
{
	uint8_t	(*read_rom)	 (struct Mapper*, uint16_t address);
	void    (*write_rom) (struct Mapper*, uint16_t address, uint8_t value);
	uint8_t	(*read_ram)	 (struct Mapper*, uint16_t address);
	void    (*write_ram) (struct Mapper*, uint16_t address, uint8_t value);
	void	(*destroy)(struct Mapper*);
	void*	state;
} Mapper;

Mapper* create_mapper(Cartridge_Type cartridge_type, const uint8_t* rom, size_t rom_size, size_t ram_size);
void destroy_mapper(Mapper* mapper);

#endif