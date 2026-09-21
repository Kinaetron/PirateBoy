#include "memory.h"
#include "cpu/cpu.h"
#include "cpu/opcodes.h"
#include "cpu/opcodes_cb.h"


void set_register_flag(Register* registers, Flag flag, bool value)
{
	if (value) {
		registers->af.low |= (1 << flag);
	}
	else {
		registers->af.low &= ~(1 << flag);
	}

	registers->af.low &= 0xF0;
}

bool get_register_flag(Register* registers, Flag flag) {
	return (registers->af.low >> flag) & 1;
}

uint8_t fetch_byte(Memory* memory, uint16_t* address)
{
	uint8_t value = memory_read(memory, *address);
	*address = *address + 1;
	return value;
}

memory16 fetch_two_bytes(Memory* memory, uint16_t* address)
{
	memory16 result;
	result.low = fetch_byte(memory, address);
	result.high = fetch_byte(memory, address);

	return result;
}

uint8_t cpu_step(Memory* memory, Register* registers)
{
	uint8_t opcode = fetch_byte(memory,  &registers->program_counter);

	if (opcode == 0xCB) {
		return opcode_cb_step(memory, registers);
	}
	else {
		return opcode_step(memory, registers, opcode);
	}
}