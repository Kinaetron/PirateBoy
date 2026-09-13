#include "memory.h"
#include "cpu/cpu.h"
#include "cpu/instruction_set_1.h"


void set_register_flag(CPU_Memory* memory, Flag flag, bool value)
{
	if (value) {
		memory->af.low |= (1 << flag);
	}
	else {
		memory->af.low &= ~(1 << flag);
	}

	memory->af.low &= 0xF0;
}

bool get_register_flag(CPU_Memory* memory, Flag flag) {
	return (memory->af.low >> flag) & 1;
}

uint8_t fetch_byte(CPU_Memory* memory, uint16_t* address)
{
	uint8_t value = memory_read(memory, *address);
	*address = *address + 1;
	return value;
}

memory16 fetch_two_bytes(CPU_Memory* memory, uint16_t* address)
{
	memory16 result;
	result.low = fetch_byte(memory, address);
	result.high = fetch_byte(memory, address);

	return result;
}

uint8_t cpu_step(CPU_Memory* memory)
{
	uint8_t opcode = fetch_byte(memory, &memory->program_counter);

	if (opcode != 0xCB) {
		return instruction_set_1_step(memory, opcode);
	}
}