#ifndef MBC0_H
#define MBC0_H

#include <stddef.h>
#include <stdint.h>
#include "mapper.h"

Mapper* create_mbc0(const uint8_t* rom, size_t rom_size);

#endif