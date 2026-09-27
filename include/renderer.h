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

void renderer_initalize(SDL_Window* window, SDL_GPUDevice*  device);
RendererTexture* renderer_create_texture(uint32_t width, uint32_t height);
void renderer_upload_texture(RendererTexture* texture, const PixelBuffer* buffer);
void renderer_render(SDL_Window* window, SDL_GPUDevice* device, RendererTexture* texture);

#endif