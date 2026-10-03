#include "renderer.h"

static SDL_GPUBuffer* vertexBuffer;
static SDL_GPUBuffer* indexBuffer;
static SDL_GPUGraphicsPipeline* pipeline;

typedef struct PositionTextureVertex
{
	float x, y;
	float u, v;
} PositionTextureVertex;

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

SDL_AppResult renderer_initialize(SDL_Window* window, SDL_GPUDevice* gpu_device)
{
	SDL_SetGPUSwapchainParameters(
		gpu_device,
		window,
		SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
		SDL_GPU_PRESENTMODE_VSYNC);

	SDL_GPUTextureFormat swapchainFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, window);
	if (swapchainFormat == SDL_GPU_TEXTUREFORMAT_INVALID) 
	{
		SDL_Log("Swapchain format is invalid! Did you forget to claim the window?");
		return SDL_APP_FAILURE;
	}

	SDL_GPUShader* vertexShader = load_shader(gpu_device, "texture.vert", 0, 0, 0, 0);
	if (vertexShader == NULL) 
	{
		SDL_Log("Failed to create vertex shader!");
		return SDL_APP_FAILURE;
	}

	SDL_GPUShader* fragmentShader = load_shader(gpu_device, "texture.frag", 1, 0, 0, 0);
	if (fragmentShader == NULL)
{
		SDL_Log("Failed to create fragment shader!");
		return SDL_APP_FAILURE;;
	}

	SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
		.target_info = {
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){ {
				.format = swapchainFormat
			} },
		},
		.vertex_input_state = (SDL_GPUVertexInputState){
			.num_vertex_buffers = 1,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){ {
				.slot = 0,
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				.instance_step_rate = 0,
				.pitch = sizeof(PositionTextureVertex)
			} },
			.num_vertex_attributes = 2,
			.vertex_attributes = (SDL_GPUVertexAttribute[]){ {
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				.location = 0,
				.offset = 0
			},{
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				.location = 1,
				.offset = sizeof(float) * 2
			} }
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader
	};

	pipeline = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipelineCreateInfo);
	if (pipeline == NULL)
	{
		SDL_Log("Failed to create pipeline!");
		return SDL_APP_FAILURE;
	}

	SDL_ReleaseGPUShader(gpu_device, vertexShader);
	SDL_ReleaseGPUShader(gpu_device, fragmentShader);

	vertexBuffer = SDL_CreateGPUBuffer(gpu_device, &(SDL_GPUBufferCreateInfo)
	{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(PositionTextureVertex) * 4
	});

	SDL_SetGPUBufferName(
		gpu_device,
		vertexBuffer,
		"Renderer Vertex Buffer"
	);

	indexBuffer = SDL_CreateGPUBuffer(gpu_device, &(SDL_GPUBufferCreateInfo){
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
			.size = sizeof(uint16_t) * 6
	});

	SDL_SetGPUBufferName(
		gpu_device,
		indexBuffer,
		"Renderer Index Buffer"
	);


	SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
		gpu_device,
		&(SDL_GPUTransferBufferCreateInfo) {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = (sizeof(PositionTextureVertex) * 4) + (sizeof(Uint16) * 6)
	});

	PositionTextureVertex* transferData = SDL_MapGPUTransferBuffer(
		gpu_device,
		transfer_buffer,
		false
	);

	transferData[0] = (PositionTextureVertex){ -1.0f,  1.0f,0.0f, 0.0f };
	transferData[1] = (PositionTextureVertex){ 1.0f,  1.0f, 1.0f, 0.0f };
	transferData[2] = (PositionTextureVertex){ 1.0f, -1.0f, 1.0f, 1.0f };
	transferData[3] = (PositionTextureVertex){ -1.0f, -1.0f,0.0f, 1.0f };

	uint16_t* indexData = (uint16_t*)&transferData[4];
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 0;
	indexData[4] = 2;
	indexData[5] = 3;

	SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buffer);

	SDL_GPUCommandBuffer* uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(gpu_device);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

	SDL_UploadToGPUBuffer(copyPass,
		&(SDL_GPUTransferBufferLocation) 
	{
		.transfer_buffer = transfer_buffer,
		.offset = 0
	},
		& (SDL_GPUBufferRegion) {
		.buffer = vertexBuffer,
		.offset = 0,
		.size = sizeof(PositionTextureVertex) * 4
	},false);

	SDL_UploadToGPUBuffer(copyPass,
		&(SDL_GPUTransferBufferLocation) {
		.transfer_buffer = transfer_buffer,
		.offset = sizeof(PositionTextureVertex) * 4
	},
		& (SDL_GPUBufferRegion) {
		.buffer = indexBuffer,
		.offset = 0,
		.size = sizeof(uint16_t) * 6
	},false);

	SDL_EndGPUCopyPass(copyPass);
	SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);
	SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buffer);

	return SDL_APP_CONTINUE;
}

RendererTexture* renderer_create_texture(uint32_t width, uint32_t height, SDL_GPUDevice* gpu_device)
{
	RendererTexture* renderer_texture = malloc(sizeof(RendererTexture));

	if (renderer_texture == NULL) {
		return NULL;
	}

	renderer_texture->width = width;
	renderer_texture->height = height;

	renderer_texture->sampler = SDL_CreateGPUSampler(gpu_device, &(SDL_GPUSamplerCreateInfo)
	{
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
	});

	renderer_texture->texture = SDL_CreateGPUTexture(gpu_device, &(SDL_GPUTextureCreateInfo)
	{
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.type = SDL_GPU_TEXTURETYPE_2D,
		.width = width,
		.height = height,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
	});

	return renderer_texture;
}

void renderer_upload_texture(RendererTexture* texture, const PixelBuffer* buffer, SDL_GPUDevice* gpu_device)
{
	size_t pixel_data_size = (size_t)texture->width * texture->height * sizeof(uint32_t);

	SDL_GPUTransferBuffer* texture_transfer_buffer = SDL_CreateGPUTransferBuffer(
		gpu_device,
		&(SDL_GPUTransferBufferCreateInfo) 
	{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = (uint32_t)pixel_data_size
	});

	void* pixels = SDL_MapGPUTransferBuffer(gpu_device, texture_transfer_buffer, false);
	SDL_memcpy(pixels, buffer->pixels, pixel_data_size);
	SDL_UnmapGPUTransferBuffer(gpu_device, texture_transfer_buffer);

	SDL_GPUCommandBuffer* upload_cmd = SDL_AcquireGPUCommandBuffer(gpu_device);
	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_cmd);

	SDL_UploadToGPUTexture(copy_pass,
		&(SDL_GPUTextureTransferInfo) 
	{
		.transfer_buffer = texture_transfer_buffer,
		.offset = 0,
	},
	& (SDL_GPUTextureRegion) 
	{
		.texture = texture->texture,
		.w = texture->width,
		.h = texture->height,
		.d = 1
	},false);

	SDL_EndGPUCopyPass(copy_pass);
	SDL_SubmitGPUCommandBuffer(upload_cmd);
	SDL_ReleaseGPUTransferBuffer(gpu_device, texture_transfer_buffer);
}

SDL_AppResult renderer_render(SDL_Window* window, SDL_GPUDevice* device, RendererTexture* texture)
{
	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);

	if (commandBuffer == NULL)
	{
		SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture* swapchainTexture;

	if (!SDL_WaitAndAcquireGPUSwapchainTexture(
		commandBuffer,
		window,
		&swapchainTexture,
		NULL,
		NULL))
	{
		SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
		SDL_CancelGPUCommandBuffer(commandBuffer);
		return SDL_APP_FAILURE;
	}

	if (swapchainTexture != NULL)
	{
		SDL_GPUColorTargetInfo target_info = { 0 };
		target_info.texture = swapchainTexture;
		target_info.store_op = SDL_GPU_STOREOP_STORE;
		target_info.clear_color = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f };
		target_info.load_op = SDL_GPU_LOADOP_CLEAR;
		target_info.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* renderPass =
			SDL_BeginGPURenderPass(commandBuffer, &target_info, 1, NULL);

		SDL_BindGPUGraphicsPipeline(renderPass, pipeline);
		SDL_BindGPUVertexBuffers(renderPass, 0, &(SDL_GPUBufferBinding){.buffer = vertexBuffer, .offset = 0 }, 1);
		SDL_BindGPUIndexBuffer(renderPass, &(SDL_GPUBufferBinding){.buffer = indexBuffer, .offset = 0 }, SDL_GPU_INDEXELEMENTSIZE_16BIT);
		SDL_BindGPUFragmentSamplers(renderPass, 0, &(SDL_GPUTextureSamplerBinding){.texture = texture->texture, .sampler = texture->sampler }, 1);
		SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
		SDL_EndGPURenderPass(renderPass);
	}

	SDL_SubmitGPUCommandBuffer(commandBuffer);
	return SDL_APP_CONTINUE;
}

void renderer_destroy_texture(RendererTexture* texture, SDL_GPUDevice* gpu_device)
{
	SDL_ReleaseGPUTexture(gpu_device, texture->texture);
	SDL_ReleaseGPUSampler(gpu_device, texture->sampler);
	free(texture);
}

void renderer_destroy(SDL_GPUDevice* gpu_device)
{
	SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline);
	SDL_ReleaseGPUBuffer(gpu_device, vertexBuffer);
	SDL_ReleaseGPUBuffer(gpu_device, indexBuffer);
}