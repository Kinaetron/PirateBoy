#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <stdbool.h>

#include "timer.h"
#include "memory.h"
#include "cpu/cpu.h"
#include "interrupt.h"
#include "ppu.h"

static SDL_Window* window = NULL;
static SDL_GPUDevice* gpu_device = NULL;

#define FPS 59.7275
#define CLOCK_HZ 4194304
#define CYCLES_PER_FRAME ((int)(CLOCK_HZ / FPS))
#define NANOSECONDS_PER_SECOND 1000000000.0

static uint64_t last_time_ns = 0;

static Memory* memory;
static Register* registers;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
	memory = calloc(1, sizeof(Memory));
	registers = calloc(1, sizeof(Register));

	if (memory == NULL || registers == NULL)
	{
		SDL_Log("Error: failed to allocate Memory/Register");
		return SDL_APP_FAILURE;
	}

	last_time_ns = SDL_GetTicksNS();

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{

	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	int cycles_this_frame = 0;

	while (cycles_this_frame < CYCLES_PER_FRAME)
	{
		if (cpu_interrupt_master_pending())
		{
			cpu_set_interrupt_master_enable(true);
			cpu_clear_interrupt_enable_pending();
		}

		uint8_t interrupt_cycles = handle_interrupts(memory, registers);

		if (interrupt_cycles != 0)
		{
			cycles_this_frame += interrupt_cycles;
			timer_step(memory, interrupt_cycles);
			ppu_step(memory, interrupt_cycles);
			continue;
		}

		uint8_t cycles = cpu_step(memory, registers);
		cycles_this_frame += cycles;

		timer_step(memory, cycles);
		ppu_step(memory, cycles);	
	}

	uint64_t target_duration_ns = (uint64_t)(((double)cycles_this_frame * NANOSECONDS_PER_SECOND) / CLOCK_HZ);
	uint64_t target_deadline_ns = last_time_ns + target_duration_ns;

	uint64_t now_ns = SDL_GetTicksNS();

	if (now_ns < target_deadline_ns) {
		SDL_DelayPrecise(target_deadline_ns - now_ns);
	}

	last_time_ns = SDL_GetTicksNS();

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	free(memory);
	free(registers);
}