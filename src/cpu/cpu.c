#include "memory.h"
#include "cpu/cpu.h"
#include "cpu/opcodes.h"
#include "cpu/opcodes_cb.h"

static bool is_halted;
static bool interrupt_master_enable;
static bool interrupt_enable_pending;

void cpu_reset_state(void)
{
	is_halted = false;
	interrupt_master_enable = false;
	interrupt_enable_pending = false;
}

bool cpu_is_halted(void) {
	return is_halted;
}

bool cpu_interrupt_master_enable(void) {
	return interrupt_master_enable;
}


bool cpu_interrupt_master_pending(void) {
	return interrupt_enable_pending;
}

void cpu_set_interrupt_master_pending(bool value) {
	interrupt_enable_pending = value;
}

void cpu_set_is_halted(bool value) {
	is_halted = value;
}

void cpu_set_interrupt_master_enable(bool value) {
	interrupt_master_enable = value;
}

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

void cpu_post_boot(Register* registers, uint8_t checksum_value)
{
	bool checksum_set = (checksum_value != 0);

	set_register_flag(registers, Z, true);
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, checksum_set);
	set_register_flag(registers, C, checksum_set);

	registers->af.high = 0x01;
	registers->bc.high = 0x00;
	registers->de.high = 0x00;
	registers->hl.high = 0x01;

	registers->bc.low = 0x13;
	registers->de.low = 0xD8;
	registers->hl.low = 0x4D;

	registers->program_counter.value = 0x0100;
	registers->stack_pointer = 0xFFFE;
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