#include <stdlib.h>

#include "cartridge/mbc0.h"
#include "cartridge/mapper.h"

Mapper* create_mapper(Cartridge_Type cartridge_type, const uint8_t* rom, size_t rom_size, size_t ram_size)
{
	switch (cartridge_type)
	{
		case CARTRIDGE_ROM_ONLY:
			return create_mbc0(rom, rom_size);

		default:
			return NULL;
	}
}

void destroy_mapper(Mapper* mapper)
{
	if (mapper == NULL) {
		return;
	}

	free(mapper->state);
	free(mapper);
}