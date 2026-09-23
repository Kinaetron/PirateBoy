#include "ppu.h"
#include "memory.h"

#define TILE_SIZE 8
#define TILE_COUNT 384
#define VRAM_START 0X8000
#define BG_SIZE 32
#define TILE_MAP_1_ADDRESS 0X9800
#define TILE_MAP_2_ADDRESS 0X9C00
#define LCD_CONTROL_ADDRES 0XFF40
#define LCD_STATUS_ADDRESS 0XFF41

typedef struct { uint8_t pixel[8][8]; } Tile;
static Tile tiles[TILE_COUNT];

typedef struct { uint8_t tile[BG_SIZE][BG_SIZE]; } BackgroundMap;

static BackgroundMap map_1;
static BackgroundMap map_2;

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
	NONE = 0,
	H_BLANK = 1,
	V_BLANK = 2,
	OAM_SCAN = 3,
	DRAWING = 4
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

static PPU_Mode get_ppu_mode(Memory* memory)
{
	switch (memory_read(memory, LCD_STATUS_ADDRESS) & 0x03)
	{
		case 0x00: return H_BLANK;
		case 0x01: return V_BLANK;
		case 0x02: return OAM_SCAN;
		case 0x03: return DRAWING;
	}

	return NONE;
}

void ppu_step(Memory* memory, uint8_t cycles)
{
	set_tile_data(memory);
	set_background_maps(memory, &map_1, TILE_MAP_1_ADDRESS);
	set_background_maps(memory, &map_2, TILE_MAP_2_ADDRESS);
}