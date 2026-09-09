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
	uint8_t flat[0x10000];

	uint8_t* rom;

	memory16 af;
	memory16 bc;
	memory16 de;
	memory16 hl;

	uint16_t stack_pointer;
	memory16 program_counter;
} CPU_Memory;

typedef enum { JoyPad = 4, Serial = 3, Timer = 2, LCD = 1, VBlank = 0 } Interrupt_Flag;

uint8_t memory_read(CPU_Memory* memory, uint16_t address);
void memory_write(CPU_Memory* memory, uint16_t address, uint8_t data);
void memory_divider_register_incrementer(CPU_Memory* memory);
void set_if_interrupt(CPU_Memory* memory, Interrupt_Flag flag, bool value);
bool is_pending(CPU_Memory* memory);

#endif