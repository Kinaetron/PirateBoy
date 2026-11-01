#ifndef MAPPER_H
#define MAPPER_H

#include <stdint.h>

typedef struct Mapper 
{
	uint8_t	(*read_rom)	 (struct Mapper*, uint16_t address);
	void    (*write_rom) (struct Mapper*, uint16_t address, uint8_t value);
	uint8_t	(*read_ram)	 (struct Mapper*, uint16_t address);
	void    (*write_ram) (struct Mapper*, uint16_t address, uint8_t value);
	void*	state;
} Mapper;

#endif