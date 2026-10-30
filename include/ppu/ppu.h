#ifndef PPU_H
#define PPU_H

#include <stdbool.h>
#include <SDL3/SDL.h>


#include "memory.h"

void ppu_post_boot(Memory* memory);
void ppu_step(Memory* memory, uint8_t cycles);
SDL_AppResult ppu_initialize(SDL_Window* window, SDL_GPUDevice* gpu_device);

#endif