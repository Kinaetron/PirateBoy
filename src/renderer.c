#include "renderer.h"

static SDL_GPUShader* load_shader(
	SDL_GPUDevice* device,
	const char* shaderFilename,
	uint32_t samplerCount,
	uint32_t uniformBufferCount,
	uint32_t storageBufferCount,
	uint32_t storageTextureCount)
{
	SDL_GPUShaderStage stage;
	if (SDL_strstr(shaderFilename, ".vert")) {
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	}
	else if (SDL_strstr(shaderFilename, ".frag")) {
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	}
	else
	{
		SDL_Log("Invalid shader stage!");
		return NULL;
	}

	char fullPath[256];
	SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char* entrypoint;

	const char* base_path = SDL_GetBasePath();

	if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) 
	{
		SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/SPIRV/%s.spv", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	}
	else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) 
	{
		SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/MSL/%s.msl", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	}
	else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) 
	{
		SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/DXIL/%s.dxil", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	}
	else 
	{
		SDL_Log("%s", "Unrecognized backend shader format!");
		return NULL;
	}

	size_t codeSize;
	void* code = SDL_LoadFile(fullPath, &codeSize);
	if (code == NULL)
	{
		SDL_Log("Failed to load shader from disk! %s", fullPath);
		return NULL;
	}

	SDL_GPUShaderCreateInfo shaderInfo = 
	{
		.code = code,
		.code_size = codeSize,
		.entrypoint = entrypoint,
		.format = format,
		.stage = stage,
		.num_samplers = samplerCount,
		.num_uniform_buffers = uniformBufferCount,
		.num_storage_buffers = storageBufferCount,
		.num_storage_textures = storageTextureCount
	};

	SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
	if (shader == NULL)
	{
		SDL_Log("Failed to create shader!");
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return shader;
}

void renderer_initalize(SDL_Window* window, SDL_GPUDevice* device) { }

RendererTexture* renderer_create_texture(uint32_t width, uint32_t height) { return NULL; }

void renderer_upload_texture(RendererTexture* texture, const PixelBuffer* buffer) { }

void renderer_render(SDL_Window* window, SDL_GPUDevice* device, RendererTexture* texture) { }