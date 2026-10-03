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

static SDL_Window* window = NULL;
static SDL_GPUDevice* gpu_device = NULL;

#define FPS 59.7275
#define CLOCK_HZ 4194304
#define CYCLES_PER_FRAME ((int)(CLOCK_HZ / FPS))
#define NANOSECONDS_PER_SECOND 1000000000.0

static uint64_t last_time_ns = 0;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
	window = SDL_CreateWindow("PirateBoy", 160, 144, 0);

	if (window == NULL)
	{
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(window);

	gpu_device = SDL_CreateGPUDevice(
		SDL_GPU_SHADERFORMAT_SPIRV |
		SDL_GPU_SHADERFORMAT_DXIL |
		SDL_GPU_SHADERFORMAT_MSL |
		SDL_GPU_SHADERFORMAT_METALLIB,
		true,
		NULL);

	if (gpu_device == NULL)
	{
		SDL_Log("Error: SDL_CreateGPUDevice: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (!SDL_ClaimWindowForGPUDevice(gpu_device, window))
	{
		SDL_Log("Error: SDL_CreateGPUDevice Window Claim: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_SetGPUSwapchainParameters(
		gpu_device,
		window,
		SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
		SDL_GPU_PRESENTMODE_IMMEDIATE);

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
	int8_t cycles_this_frame = 0;

	while (cycles_this_frame < CYCLES_PER_FRAME)
	{
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
}