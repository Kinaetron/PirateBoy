#ifndef CPU_MEMORY
#define CPU_MEMORY

#include <cstdint>

#define CPU_MEMORY_SIZE 8192

typedef struct
{
	uint16_t* rom;
	uint16_t ram[CPU_MEMORY_SIZE];
	uint16_t af;
	uint16_t bc;
	uint16_t de;
	uint16_t hl;
	uint16_t stack_pointer;
	uint16_t program_counter;
} CPU_Memory;

#endif