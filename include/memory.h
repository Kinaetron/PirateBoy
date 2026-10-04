#ifndef CPU_MEMORY
#define CPU_MEMORY

#include <stdint.h>
#include <stdbool.h>

#define IO_SIZE 128
#define HRAM_SIZE 127
#define WRAM_SIZE 8192

#define ROM_START          0x0000
#define ROM_END            0x7FFF

#define VRAM_START         0x8000
#define VRAM_END           0x9FFF

#define CART_RAM_START     0xA000
#define CART_RAM_END       0xBFFF

#define WRAM_START         0xC000
#define WRAM_END           0xDFFF

#define ECHO_RAM_START     0xE000
#define ECHO_RAM_END       0xFDFF

#define OAM_START          0xFE00
#define OAM_END            0xFE9F

#define IO_START           0xFF00
#define IO_END             0xFF7F

#define HRAM_START         0xFF80
#define HRAM_END           0xFFFE

#define INTERRUPT_ENABLE_ADDR 0xFFFF

#define INTERRUPT_FLAG_ADDR 0xFF0F

#define MMU_UNMAPPED_READ_VALUE 0xFF

#define DIVIDER_REGISTER	0XFF04

typedef enum
{
	CARTRIDGE_ROM_ONLY = 0x00,
	CARTRIDGE_MBC1 = 0x01,
	CARTRIDGE_MBC1_RAM = 0x02,
	CARTRIDGE_MBC1_RAM_BATTERY = 0x03,
	CARTRIDGE_MBC2 = 0x05,
	CARTRIDGE_MBC2_BATTERY = 0x06,
	CARTRIDGE_ROM_RAM = 0x08,
	CARTRIDGE_ROM_RAM_BATTERY = 0x09,
	CARTRIDGE_MMM01 = 0x0B,
	CARTRIDGE_MMM01_RAM = 0x0C,
	CARTRIDGE_MMM01_RAM_BATTERY = 0x0D,
	CARTRIDGE_MBC3_TIMER_BATTERY = 0x0F,
	CARTRIDGE_MBC3_TIMER_RAM_BATTERY = 0x10,
	CARTRIDGE_MBC3 = 0x11,
	CARTRIDGE_MBC3_RAM = 0x12,
	CARTRIDGE_MBC3_RAM_BATTERY = 0x13,
	CARTRIDGE_MBC5 = 0x19,
	CARTRIDGE_MBC5_RAM = 0x1A,
	CARTRIDGE_MBC5_RAM_BATTERY = 0x1B,
	CARTRIDGE_MBC5_RUMBLE = 0x1C,
	CARTRIDGE_MBC5_RUMBLE_RAM = 0x1D,
	CARTRIDGE_MBC5_RUMBLE_RAM_BATTERY = 0x1E,
	CARTRIDGE_MBC6 = 0x20,
	CARTRIDGE_MBC7_SENSOR_RUMBLE_RAM_BATTERY = 0x22,
	CARTRIDGE_POCKET_CAMERA = 0xFC,
	CARTRIDGE_BANDAI_TAMA5 = 0xFD,
	CARTRIDGE_HUC3 = 0xFE,
	CARTRIDGE_HUC1_RAM_BATTERY = 0xFF
}Cartridge_Type;

typedef struct
{
	char title[16];
	char manufacturer_code[5];
	uint8_t cgb_flag;
	uint8_t licensee_code[2];
	uint8_t sgb_flag;
	Cartridge_Type cartridge_type;
	uint8_t rom_size;
	uint8_t ram_size;
	uint8_t header_checksum;
	uint8_t* data;
	size_t size;
} Rom;

typedef struct
{
	uint8_t flat[0x10000];
	Rom* rom;
} Memory;

typedef union
{
	uint16_t value;
	struct
	{
		uint8_t low;
		uint8_t high;
	};
} memory16;

typedef struct
{
	memory16 af;
	memory16 bc;
	memory16 de;
	memory16 hl;

	uint16_t stack_pointer;
	memory16 program_counter;
} Register;

typedef enum { JoyPad = 4, Serial = 3, Timer = 2, LCD = 1, VBlank = 0 } Interrupt_Flag;

uint8_t memory_read(Memory* memory, uint16_t address);
void memory_write(Memory* memory, uint16_t address, uint8_t data);
void memory_divider_register_incrementer(Memory* memory);
void set_if_interrupt(Memory* memory, Interrupt_Flag flag, bool value);
bool is_pending(Memory* memory);
bool load_rom(Memory* memory, const char* path);
void unload_rom(Memory* memory);
void memory_post_boot(Memory* memory);

#endif