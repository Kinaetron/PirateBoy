#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>

#include "renderer.h"

static SDL_Window* window = NULL;
static SDL_GPUDevice* gpu_device = NULL;

bool renderer_initialization(const char* title, int width, int height) 
{
	window = SDL_CreateWindow(title, width, height, 0);

	if (window == NULL)
	{
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		return false;
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
		return false;
	}

	if (!SDL_ClaimWindowForGPUDevice(gpu_device, window))
	{
		SDL_Log("Error: SDL_CreateGPUDevice Window Claim: %s", SDL_GetError());
		return false;
	}

	SDL_SetGPUSwapchainParameters(
		gpu_device,
		window,
		SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
		SDL_GPU_PRESENTMODE_IMMEDIATE);

	return true;
}