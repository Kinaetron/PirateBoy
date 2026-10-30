#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>
#include <SDL3/SDL.h>

typedef struct
{
	uint8_t r, g, b, a;
} Pixel;

typedef struct
{
	uint32_t width;
	uint32_t height;
	Pixel* pixels;
} PixelBuffer;

typedef struct
{
	SDL_GPUTexture* texture;
	SDL_GPUSampler* sampler;
	uint32_t width;
	uint32_t height;
} RendererTexture;

SDL_AppResult renderer_initialize(SDL_Window* window, SDL_GPUDevice* gpu_device);
RendererTexture* renderer_create_texture(uint32_t width, uint32_t height, SDL_GPUDevice* gpu_device);
void renderer_upload_texture(RendererTexture* texture, const PixelBuffer* buffer, SDL_GPUDevice* gpu_device);
SDL_AppResult renderer_render(SDL_Window* window, SDL_GPUDevice* gpu_device, RendererTexture* texture);
void renderer_destroy_texture(RendererTexture* texture, SDL_GPUDevice* gpu_device);
void renderer_destroy(SDL_GPUDevice* gpu_device);

#endif