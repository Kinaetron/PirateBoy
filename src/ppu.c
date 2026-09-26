#include "ppu.h"
#include "memory.h"

#include <stdbool.h>

#define TILE_MAP_1_ADDRESS 0X9800
#define TILE_MAP_2_ADDRESS 0X9C00
#define LCD_CONTROL_ADDRES 0XFF40
#define LCD_STATUS_ADDRESS 0XFF41
#define OBJECT_ATTRIBUTE_ADDRESS 0XFE00
#define LINE_Y_ADDRESS 0xFF44
#define LYC_ADDRESS 0xFF45

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

typedef struct { uint8_t pixel[8][8]; } Tile;
static Tile tiles[TILE_COUNT];

typedef struct { uint8_t tile[BG_SIZE][BG_SIZE]; } BackgroundMap;

static uint32_t line_dots = 0;
static PPU_Mode previous_mode = H_BLANK;

typedef enum
{
	OBP0 = 0,
	OBP1 = 1
} Palette;

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

static BackgroundMap map_1;
static BackgroundMap map_2;
static Sprite_Attributes sprites[OAM_ENTRY_COUNT];
static Sprite_Attributes sprite_buffer[OAM_BUFFER_LIMIT];

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

static bool get_lcd_control_register(Memory* memory, LCD_Flag flag)  {
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

static void oam_scan_mode(Memory* memory, uint8_t line)
{
	uint8_t count = 0;
	uint8_t obj_height = 8;
	bool obj_size_type = get_lcd_control_register(memory, LCD_OBJ_SIZE);

	if (obj_size_type) {
		obj_height = 16;
	}

	for (int i = 0; i < OAM_ENTRY_COUNT; i++)
	{
		if (sprites[i].x <= 0) {
			continue;
		}

		if (line< sprites[i].y) {
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
				// this is where we're going to render
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

		if (line >= TOTAL_LINES) {
			line = 0;
		}

		memory_write(memory, LINE_Y_ADDRESS, line);

		if (line == LAST_DRAW_LINE) {
			set_if_interrupt(memory, VBlank, true);
		}

		check_lyc(memory, line);
	}
}