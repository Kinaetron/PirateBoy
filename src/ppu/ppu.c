#include "ppu/ppu.h"
#include "ppu/renderer.h"
#include "ppu/pixel_queue.h"

#include <string.h>
#include <stdbool.h>

#define TILE_MAP_1_ADDRESS 0X9800
#define TILE_MAP_2_ADDRESS 0X9C00
#define LCD_CONTROL_ADDRES 0XFF40
#define LCD_STATUS_ADDRESS 0XFF41
#define OBJECT_ATTRIBUTE_ADDRESS 0XFE00
#define LINE_Y_ADDRESS 0xFF44
#define LYC_ADDRESS 0xFF45
#define SCROLL_Y_ADDRESS 0xFF42
#define SCROLL_X_ADDRESS 0xFF43
#define BG_PALETTE_ADDRESS 0xFF47
#define OBP0_PALETTE_ADDRESS 0xFF48
#define OBP1_PALETTE_ADDRESS 0xFF49
#define WINDOW_Y_ADDRESS 0xFF4A
#define WINDOW_X_ADDRESS 0xFF4B
#define WINDOW_X_OFFSET 7

#define TILE_SIZE 8
#define TILE_COUNT 384
#define VRAM_START 0X8000
#define BG_SIZE 32
#define DOTS_PER_LINE 456
#define TOTAL_LINES   154
#define OAM_ENTRY_COUNT 40

#define V_BLANK_BOUNDARY 144
#define OAM_SCAN_BOUNDARY 80
#define DRAWING_BOUNDARY 252

#define LAST_DRAW_LINE 144

#define OAM_BUFFER_LIMIT 10

#define TEXTURE_WIDTH 160
#define TEXTURE_HEIGHT 144

#define SIGNED_TILE_BASE 256

typedef struct
{
	bool priority;
	bool y_flip;
	bool x_flip;
	Palette sprite_palette;
} Sprite_Flags;

typedef struct
{
	int16_t y;
	int16_t x;
	uint8_t tile_index;
	Sprite_Flags flags;
} Sprite_Attributes;

typedef enum
{
	LCD_BG_ENABLE = 0,
	LCD_OBJ_ENABLE = 1,
	LCD_OBJ_SIZE = 2,
	LCD_BG_TILE_MAP = 3,
	LCD_TILE_DATA = 4,
	LCD_WINDOW_ENABLE = 5,
	LCD_WINDOW_TILE_MAP = 6,
	LCD_ENABLE = 7
} LCD_Flag;

typedef enum
{
	LCD_STAT_LYC = 2,
	LCD_STAT_MODE_0_INT = 3,
	LCD_STAT_MODE_1_INT = 4,
	LCD_STAT_MODE_2_INT = 5,
	LCD_STAT_LYC_INT = 6
} LCD_Status_Flag;

typedef enum
{
	H_BLANK = 0,
	V_BLANK = 1,
	OAM_SCAN = 2,
	DRAWING = 3
} PPU_Mode;


static const Pixel shade_colors[4] =
{
	{ .r = 0xDF, .g = 0xEE, .b = 0xCB, .a = 0xFF },
	{ .r = 0x9B, .g = 0xBC, .b = 0x59, .a = 0xFF },
	{ .r = 0x4B, .g = 0x7A, .b = 0x3C, .a = 0xFF },
	{ .r = 0x18, .g = 0x28, .b = 0x0F, .a = 0xFF }
};

typedef struct { uint8_t pixel[8][8]; } Tile;

typedef struct { uint8_t tile[BG_SIZE][BG_SIZE]; } BackgroundMap;

static PPU_Mode previous_mode = H_BLANK;

static BackgroundMap map_1;
static BackgroundMap map_2;
static Sprite_Attributes sprites[OAM_ENTRY_COUNT];
static Sprite_Attributes sprite_buffer[OAM_BUFFER_LIMIT];

static Tile tiles[TILE_COUNT];

static uint32_t line_dots = 0;

static PixelBuffer pixel_buffer;
static RendererTexture* renderer_texture;

static PixelQueue* object_queue;
static PixelQueue* background_queue;


static uint8_t screen_x;
static uint8_t fetcher_column;
static uint8_t sprite_count;

static bool window_active;
uint8_t window_column;
uint8_t window_line_counter;

static SDL_Window* ppu_window;
static SDL_GPUDevice* ppu_gpu_device;


static void set_tile_data(Memory* memory)
{
	uint16_t address = VRAM_START;

	for (int tile = 0; tile < TILE_COUNT; tile++)
	{
		for (int y = 0; y < TILE_SIZE; y++)
		{
			uint8_t low = memory_read(memory, address++);
			uint8_t high = memory_read(memory, address++);

			for (int x = 0; x < TILE_SIZE; x++)
			{
				int bit = 7 - x;

				tiles[tile].pixel[y][x] =
					(((high >> bit) & 1) << 1) |
					((low >> bit) & 1);
			}
		}
	}
}

static void set_background_maps(Memory* memory, BackgroundMap* map, uint16_t address)
{
	for (int y = 0; y < BG_SIZE; y++)
	{
		for (int x = 0; x < BG_SIZE; x++) {
			map->tile[y][x] = memory_read(memory, address++);;
		}
	}
}

static bool get_lcd_control_register(Memory* memory, LCD_Flag flag) {
	return (memory_read(memory, LCD_CONTROL_ADDRES) >> flag) & 0x01;
}

static bool get_lcd_status_register(Memory* memory, LCD_Status_Flag flag) {
	return (memory_read(memory, LCD_STATUS_ADDRESS) >> flag) & 0x01;
}

static void set_lcd_status_register(Memory* memory, LCD_Status_Flag flag, bool value)
{
	uint8_t status = memory_read(memory, LCD_STATUS_ADDRESS);

	if (value) {
		status |= (1 << flag);
	}
	else {
		status &= ~(1 << flag);
	}

	memory_write(memory, LCD_STATUS_ADDRESS, status);
}

static void set_ppu_mode(Memory* memory, PPU_Mode mode)
{
	uint8_t status = memory_read(memory, LCD_STATUS_ADDRESS);
	status = (status & 0xFC) | mode;

	memory_write(memory, LCD_STATUS_ADDRESS, status);
}

static uint8_t get_attribute_flags(uint8_t value, uint8_t bit) {
	return (value >> bit) & 0x01;
}

static void get_sprite_attributes(Memory* memory)
{
	uint16_t address = OBJECT_ATTRIBUTE_ADDRESS;

	for (int i = 0; i < OAM_ENTRY_COUNT; i++)
	{
		sprites[i].y = memory_read(memory, address++) - 16;
		sprites[i].x = memory_read(memory, address++) - 8;
		sprites[i].tile_index = memory_read(memory, address++);


		uint8_t flags = memory_read(memory, address++);
		sprites[i].flags.priority = (bool)get_attribute_flags(flags, 7);
		sprites[i].flags.y_flip = (bool)get_attribute_flags(flags, 6);
		sprites[i].flags.x_flip = (bool)get_attribute_flags(flags, 5);
		sprites[i].flags.sprite_palette = (Palette)get_attribute_flags(flags, 4);
	}
}

static void check_lyc(Memory* memory, uint8_t line)
{
	uint8_t lyc = memory_read(memory, LYC_ADDRESS);
	bool match = (line == lyc);

	set_lcd_status_register(memory, LCD_STAT_LYC, match);

	if (match && get_lcd_status_register(memory, LCD_STAT_LYC_INT)) {
		set_if_interrupt(memory, LCD, true);
	}
}

static PPU_Mode compute_mode(uint8_t line_number, uint32_t line_dots)
{
	if (line_number >= V_BLANK_BOUNDARY) {
		return V_BLANK;
	}
	else  if (line_dots < OAM_SCAN_BOUNDARY) {
		return OAM_SCAN;
	}
	else if (line_dots < DRAWING_BOUNDARY) {
		return DRAWING;
	}
	else {
		return H_BLANK;
	}
}

static uint8_t get_pixel_shade(uint8_t palette, uint8_t color) {
	return (palette >> (color * 2)) & 0x03;
}

static void oam_scan_mode(Memory* memory, uint8_t line)
{
	uint8_t count = 0;
	uint8_t obj_height = 8;
	bool obj_size_type = get_lcd_control_register(memory, LCD_OBJ_SIZE);
	get_sprite_attributes(memory);

	if (obj_size_type) {
		obj_height = 16;
	}

	memset(sprite_buffer, 0, sizeof(sprite_buffer));

	for (int i = 0; i < OAM_ENTRY_COUNT; i++)
	{
		if (sprites[i].x <= -8) {
			continue;
		}

		if (line < sprites[i].y) {
			continue;
		}

		if (line >= (sprites[i].y + obj_height)) {
			continue;
		}

		sprite_buffer[count++] = sprites[i];

		if (count >= OAM_BUFFER_LIMIT) {
			break;
		}
	}

	sprite_count = count;
}

static void background_queue_filler(Memory* memory)
{
	if (!is_queue_empty(background_queue)) {
		return;
	}

	uint8_t line_y = memory_read(memory, LINE_Y_ADDRESS);
	uint8_t scroll_x = memory_read(memory, SCROLL_X_ADDRESS);
	uint8_t scroll_y = memory_read(memory, SCROLL_Y_ADDRESS);

	uint8_t row = ((line_y + scroll_y) & (BG_SIZE * TILE_SIZE - 1)) / TILE_SIZE;
	uint8_t column = ((scroll_x / TILE_SIZE) + fetcher_column) & (BG_SIZE - 1);

	BackgroundMap* map = get_lcd_control_register(memory, LCD_BG_TILE_MAP) ? &map_2 : &map_1;
	uint8_t map_index = map->tile[row][column];

	uint16_t tile_number = get_lcd_control_register(memory, LCD_TILE_DATA)
		? map_index
		: SIGNED_TILE_BASE + (int8_t)map_index;

	uint8_t tile_row = (line_y + scroll_y) & (TILE_SIZE - 1);

	for (uint8_t i = 0; i < TILE_SIZE; i++) 
	{
		PixelInfo pixel = 
		{ 
			.color = tiles[tile_number].pixel[tile_row][i], 
			.palette = OBP0,
			.bg_priority = false 
		};

		enqueue(background_queue, &pixel);
	}

	fetcher_column++;
}

static void window_queue_filler(Memory* memory)
{
	if (!is_queue_empty(background_queue)) {
		return;
	}

	uint8_t row = window_line_counter / TILE_SIZE;
	uint8_t column = window_column;

	BackgroundMap* map = get_lcd_control_register(memory, LCD_WINDOW_TILE_MAP) ? &map_2 : &map_1;
	uint8_t map_index = map->tile[row][column];

	uint16_t tile_number = get_lcd_control_register(memory, LCD_TILE_DATA)
		? map_index
		: SIGNED_TILE_BASE + (int8_t)map_index;

	uint8_t tile_row = window_line_counter & (TILE_SIZE - 1);

	for (uint8_t i = 0; i < TILE_SIZE; i++)
	{
		PixelInfo pixel =
		{
			.color = tiles[tile_number].pixel[tile_row][i],
			.palette = OBP0,
			.bg_priority = false
		};

		enqueue(background_queue, &pixel);
	}

	window_column++;
}

static void object_queue_filler(Memory* memory)
{
	for (uint8_t i = 0; i < sprite_count; i++)
	{
		Sprite_Attributes sprite = sprite_buffer[i];

		if (sprite.x != screen_x) {
			continue;
		}

		bool obj_size_type = get_lcd_control_register(memory, LCD_OBJ_SIZE);
		uint8_t height = obj_size_type ? 16 : 8;

		uint8_t line_y = memory_read(memory, LINE_Y_ADDRESS);
		uint8_t row = line_y - sprite.y;
		row = sprite.flags.y_flip ? (height - 1 - row) : row;

		uint16_t tile_number = sprite.tile_index;

		if (obj_size_type)
		{
			tile_number &= 0xFE;

			if (row >= TILE_SIZE)
			{
				tile_number++;
				row -= TILE_SIZE;
			}
		}

		PixelInfo transparent = { 0 };
		while (!is_queue_full(object_queue)) {
			enqueue(object_queue, &transparent);
		}

		for (uint8_t pixel_index = 0; pixel_index < TILE_SIZE; pixel_index++)
		{
			uint8_t column = sprite.flags.x_flip ? (TILE_SIZE - 1 - pixel_index) : pixel_index;
			uint8_t color = tiles[tile_number].pixel[row][column];

			if (color == 0) {
				continue;
			}

			PixelInfo existing;
			if (queue_get(object_queue, pixel_index, &existing) && existing.color == 0)
			{
				PixelInfo pixel =
				{
					.color = color,
					.palette = sprite.flags.sprite_palette,
					.bg_priority = sprite.flags.priority
				};

				queue_set(object_queue, pixel_index, &pixel);
			}
		}
	}
}

static void pixel_mixer(Memory* memory)
{
	if (is_queue_empty(background_queue)) {
		return;
	}

	PixelInfo bg_pixel;
	PixelInfo object_pixel;

	dequeue(background_queue, &bg_pixel);

	if (!dequeue(object_queue, &object_pixel)) {
		object_pixel = (PixelInfo){ 0 };
	}

	if (!get_lcd_control_register(memory, LCD_BG_ENABLE)) {
		bg_pixel.color = 0;
	}

	if (!get_lcd_control_register(memory, LCD_OBJ_ENABLE)) {
		object_pixel.color = 0;
	}

	uint8_t shade;

	if (object_pixel.color == 0 || 
		(object_pixel.bg_priority && bg_pixel.color != 0))
	{
		uint8_t palette_register = memory_read(memory, BG_PALETTE_ADDRESS);
		shade = get_pixel_shade(palette_register, bg_pixel.color);
	}
	else
	{
		uint8_t palette_register = object_pixel.palette == OBP0 ?
			memory_read(memory, OBP0_PALETTE_ADDRESS) :
			memory_read(memory, OBP1_PALETTE_ADDRESS);

		shade = get_pixel_shade(palette_register, object_pixel.color);
	}

	uint8_t line_y = memory_read(memory, LINE_Y_ADDRESS);
	pixel_buffer.pixels[line_y * TEXTURE_WIDTH + screen_x] = shade_colors[shade];
	screen_x++;
}

static void drawing_mode(Memory* memory)
{
	screen_x = 0;
	fetcher_column = 0;
	window_active = false;

	PixelInfo discard;
	while (dequeue(object_queue, &discard)) { }
	while (dequeue(background_queue, &discard)) { }

	while (screen_x < TEXTURE_WIDTH)
	{
		bool window_enabled = get_lcd_control_register(memory, LCD_WINDOW_ENABLE);
		uint8_t line_y = memory_read(memory, LINE_Y_ADDRESS);
		uint8_t window_x = memory_read(memory, WINDOW_X_ADDRESS);
		uint8_t window_y = memory_read(memory, WINDOW_Y_ADDRESS);

		if (!window_active &&
			window_enabled &&
			line_y >= window_y &&
			screen_x + WINDOW_X_OFFSET >= window_x)
		{
			window_active = true;
			window_column = 0;
			while (dequeue(background_queue, &discard)) { }
		}

		object_queue_filler(memory);

		if (window_active) {
			window_queue_filler(memory);
		}
		else {
			background_queue_filler(memory);
		}
		pixel_mixer(memory);
	}

	if (window_active) {
		window_line_counter++;
	}
}

SDL_AppResult ppu_initialize(SDL_Window* window, SDL_GPUDevice* gpu_device)
{
	ppu_window = window;
	ppu_gpu_device = gpu_device;

	SDL_AppResult renderer_init = renderer_initialize(ppu_window, ppu_gpu_device);

	if (renderer_init == SDL_APP_FAILURE) {
		return renderer_init;
	}

	renderer_texture = renderer_create_texture(TEXTURE_WIDTH, TEXTURE_HEIGHT, ppu_gpu_device);

	if (renderer_texture == NULL) 
	{
		SDL_Log("Failed to create renderer texture");
		return SDL_APP_FAILURE;
	}

	pixel_buffer.width = TEXTURE_WIDTH;
	pixel_buffer.height = TEXTURE_HEIGHT;
	pixel_buffer.pixels = calloc(TEXTURE_WIDTH * TEXTURE_HEIGHT, sizeof(Pixel));

	if (pixel_buffer.pixels == NULL) 
	{
		SDL_Log("Failed to create pixel buffer");
		return SDL_APP_FAILURE;
	}

	object_queue = create_queue(8);

	if (object_queue == NULL)
	{
		SDL_Log("Failed to create object queue.");
		return SDL_APP_FAILURE;
	}

	background_queue = create_queue(8);

	if (background_queue == NULL)
	{
		SDL_Log("Failed to create object queue.");
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

void ppu_post_boot(Memory* memory)
{
	line_dots = 0;
	previous_mode = V_BLANK;

	memory->flat[LCD_CONTROL_ADDRES] = 0x91;
	memory->flat[LCD_STATUS_ADDRESS] = 0x85;
	memory->flat[SCROLL_Y_ADDRESS] = 0x00;
	memory->flat[SCROLL_X_ADDRESS] = 0x00;
	memory->flat[LINE_Y_ADDRESS] = 0x00;
	memory->flat[LYC_ADDRESS] = 0x00;
	memory->flat[BG_PALETTE_ADDRESS] = 0xFC;
	memory->flat[WINDOW_Y_ADDRESS] = 0x00;
	memory->flat[WINDOW_X_ADDRESS] = 0x00;
}

void ppu_step(Memory* memory, uint8_t cycles)
{
	if (memory->vram_dirty)
	{
		set_tile_data(memory);
		set_background_maps(memory, &map_1, TILE_MAP_1_ADDRESS);
		set_background_maps(memory, &map_2, TILE_MAP_2_ADDRESS);
		memory->vram_dirty = false;
	}

	line_dots += cycles;

	uint8_t line = memory_read(memory, LINE_Y_ADDRESS);

	PPU_Mode current_mode = compute_mode(line, line_dots);

	if (current_mode != previous_mode)
	{
		switch (current_mode)
		{
		case OAM_SCAN:
			oam_scan_mode(memory, line);
			break;
		case DRAWING:
			drawing_mode(memory);
			break;
		case H_BLANK:
			break;
		case V_BLANK:
			break;
		default:
			break;
		}

		set_ppu_mode(memory, current_mode);
		previous_mode = current_mode;
	}

	if (line_dots >= DOTS_PER_LINE)
	{
		line_dots -= DOTS_PER_LINE;

		line++;

		if (line >= TOTAL_LINES) 
		{
			line = 0;
			window_line_counter = 0;
		}

		memory_write(memory, LINE_Y_ADDRESS, line);

		if (line == LAST_DRAW_LINE) 
		{
			set_if_interrupt(memory, VBlank, true);

			renderer_upload_texture(renderer_texture, &pixel_buffer, ppu_gpu_device);
			renderer_render(ppu_window, ppu_gpu_device, renderer_texture);
		}

		check_lyc(memory, line);
	}
}