#ifndef PPU_H
#define PPU_H

#include <stdbool.h>
#include <SDL3/SDL.h>
#include "memory.h"

typedef enum
{
	OBP0 = 0,
	OBP1 = 1
} Palette;

void ppu_post_boot(Memory* memory);
void ppu_step(Memory* memory, uint8_t cycles);
SDL_AppResult ppu_initialize(SDL_Window* window, SDL_GPUDevice* gpu_device);

#endif