#ifndef CPU_MEMORY
#define CPU_MEMORY

#include <stdint.h>

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

#define MMU_UNMAPPED_READ_VALUE 0xFF

typedef union
{
	uint16_t value;
	struct
	{
		uint8_t low;
		uint8_t high;
	};
} Register16;

typedef struct
{
	uint8_t* rom;

	uint8_t wram[WRAM_SIZE];
	uint8_t input_output[IO_SIZE];
	uint8_t hram[HRAM_SIZE];
	uint8_t interrupt_enable;

	Register16 af;
	Register16 bc;
	Register16 de;
	Register16 hl;

	uint16_t stack_pointer;
	uint16_t program_counter;
} CPU_Memory;

#endif