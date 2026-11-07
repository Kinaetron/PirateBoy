#ifndef MBC1_H
#define MBC1_H

#include <stddef.h>
#include <stdint.h>
#include "mapper.h"

Mapper* create_mbc1(const uint8_t* rom, size_t rom_size, size_t ram_size);

#endif