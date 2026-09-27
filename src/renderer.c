#include "renderer.h"

void renderer_initalize(SDL_Window* window, SDL_GPUDevice* device) { }

RendererTexture* renderer_create_texture(uint32_t width, uint32_t height) { return NULL; }

void renderer_upload_texture(RendererTexture* texture, const PixelBuffer* buffer) { }

void renderer_render(SDL_Window* window, SDL_GPUDevice* device, RendererTexture* texture) { }