#include "memory.h"
#include "cpu/cpu.h"
#include "cpu/opcodes.h"

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

void cpu_set_interrupt_master_enable(bool value) {
	interrupt_master_enable = value;
}

static bool get_ie_interrupt(Memory* memory, Interrupt_Flag flag) {
	return (memory->flat[INTERRUPT_ENABLE_ADDR] >> flag) & 1;
}

static void set_ie_interrupt(Memory* memory, Interrupt_Flag flag, bool value)
{
	if (value) {
		memory->flat[INTERRUPT_ENABLE_ADDR] |= (1 << flag);
	}
	else {
		memory->flat[INTERRUPT_ENABLE_ADDR] &= ~(1 << flag);
	}
}

static bool get_if_interrupt(Memory* memory, Interrupt_Flag flag) {
	return (memory->flat[INTERRUPT_FLAG_ADDR] >> flag) & 1;
}

static void write_byte(Memory* memory, uint16_t* address, uint8_t data)
{
	*address = *address - 1;
	memory_write(memory, *address, data);
}

static uint8_t add_opcode(Register* registers, uint8_t value)
{
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, ((registers->af.high & 0x0F) + (value & 0x0F)) > 0x0F);
	set_register_flag(registers, C, ((uint16_t)registers->af.high + value) > 0xFF);

	registers->af.high += value;

	set_register_flag(registers, Z, registers->af.high == 0);

	return 4;
}

static uint8_t add_c_opcode(Register* registers, uint8_t value)
{
	uint8_t c_flag = (uint8_t)get_register_flag(registers, C);

	set_register_flag(registers, N, false);
	set_register_flag(registers, H, ((registers->af.high & 0x0F) + (value & 0x0F) + c_flag) > 0x0F);
	set_register_flag(registers, C, ((uint16_t)registers->af.high + value + c_flag) > 0xFF);


	registers->af.high += value + c_flag;

	set_register_flag(registers, Z, registers->af.high == 0);

	return 4;
}

static uint8_t subtract_opcode(Register* registers, uint8_t value)
{
	set_register_flag(registers, N, true);
	set_register_flag(registers, H, (value & 0x0F) > (registers->af.high & 0x0F));
	set_register_flag(registers, C, value > registers->af.high);

	registers->af.high -= value;

	set_register_flag(registers, Z, registers->af.high == 0);

	return 4;
}

static uint8_t subtract_c_opcode(Register* registers, uint8_t value)
{
	uint8_t c_flag = (uint8_t)get_register_flag(registers, C);

	set_register_flag(registers, N, true);
	set_register_flag(registers, H, ((value & 0x0F) + c_flag) > (registers->af.high & 0x0F));
	set_register_flag(registers, C, (value + c_flag) > registers->af.high);

	registers->af.high -= value + c_flag;

	set_register_flag(registers, Z, registers->af.high == 0);

	return 4;
}

static uint8_t and_opcode(Register* registers, uint8_t value)
{
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, true);
	set_register_flag(registers, C, false);

	registers->af.high &= value;

	set_register_flag(registers, Z, registers->af.high == 0);

	return 4;
}

static uint8_t xor_opcode(Register* registers, uint8_t value)
{
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, false);
	set_register_flag(registers, C, false);

	registers->af.high ^= value;

	set_register_flag(registers, Z, registers->af.high == 0);

	return 4;
}

static uint8_t or_opcode(Register* registers, uint8_t value)
{
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, false);
	set_register_flag(registers, C, false);

	registers->af.high |= value;

	set_register_flag(registers, Z, registers->af.high == 0);

	return 4;
}

static uint8_t compare_opcode(Register* registers, uint8_t value)
{
	set_register_flag(registers, N, true);
	set_register_flag(registers, H, (value & 0x0F) > (registers->af.high & 0x0F));
	set_register_flag(registers, C, value > registers->af.high);

	uint8_t compare_value = registers->af.high - value;

	set_register_flag(registers, Z, compare_value == 0);

	return 4;
}

static uint8_t increment_opcode(Memory* memory, Register* registers, uint8_t* value)
{
	uint8_t a = *value;

	set_register_flag(registers, H, (a & 0x0F) == 0x0F);
	set_register_flag(registers, N, false);

	a++;
	*value = a;

	set_register_flag(registers, Z, a == 0x00);

	return 4;
}

static uint8_t decrement_opcode(Memory* memory,Register* registers, uint8_t* value)
{
	uint8_t a = *value;

	set_register_flag(registers, H, (a & 0x0F) == 0x00);
	set_register_flag(registers, N, true);

	a--;
	*value = a;

	set_register_flag(registers, Z, a == 0x00);

	return 4;
}

static uint8_t restart_opcode(Memory* memory, Register* registers, uint8_t low)
{
	write_byte(memory, &registers->stack_pointer, registers->program_counter.high);
	write_byte(memory, &registers->stack_pointer, registers->program_counter.low);

	registers->program_counter.low = low;
	registers->program_counter.high = 0x00;

	return 16;
}

static void do_return_opcode(Memory* memory, Register* registers) {
	registers->program_counter = fetch_two_bytes(memory, &registers->stack_pointer);
}

static uint8_t conditional_return_opcode(Memory* memory, Register* registers, Flag flag, bool expected)
{
	bool flag_value = get_register_flag(memory, flag);

	if (flag_value == expected)
	{
		do_return_opcode(memory, registers);
		return 20;
	}

	return 8;
}

static uint8_t call_opcode(Memory* memory, Register* registers, memory16 jump_target)
{
	write_byte(memory, &registers->stack_pointer, registers->program_counter.high);
	write_byte(memory, &registers->stack_pointer, registers->program_counter.low);

	registers->program_counter = jump_target;

	return 24;
}

static uint8_t conditional_call_opcode(Memory* memory, Register* registers, Interrupt_Flag flag, bool expected)
{
	bool flag_value = get_register_flag(memory, flag);
	memory16 jump_target = fetch_two_bytes(memory, &registers->program_counter);

	if (flag_value == expected) {
		return call_opcode(memory, registers, jump_target);
	}

	return 12;
}

static uint8_t conditional_jump_relative_opcode(Memory* memory, Register* registers, Flag flag, bool expected)
{
	int8_t offset = (int8_t)fetch_byte(memory, &registers->program_counter);
	bool flag_value = get_register_flag(memory, flag);

	if (flag_value == expected)
	{
		registers->program_counter.value += offset;
		return 12;
	}

	return 8;
}

static uint8_t conditional_jump_absolute_opcode(Memory* memory, Register* registers, Flag flag, bool expected)
{
	memory16 target = fetch_two_bytes(memory, &registers->program_counter);
	bool flag_value = get_register_flag(memory, flag);

	if (flag_value == expected)
	{
		registers->program_counter = target;
		return 16;
	}

	return 12;
}

static uint8_t opcode_0x01(Memory* memory, Register* registers)
{
	registers->bc.value = fetch_two_bytes(memory, &registers->program_counter).value;

	return 12;
}

static uint8_t opcode_0x02(Memory* memory, Register* registers)
{
	uint8_t a_register = registers->af.high;
	memory_write(memory, registers->bc.value, a_register);

	return 8;
}

static uint8_t opcode_0x03(Register* registers)
{
	registers->bc.value++;

	return 8;
}

static uint8_t opcode_0x04(Memory* memory, Register* registers) {
	return increment_opcode(memory, registers, &registers->bc.high);
}

static uint8_t opcode_0x05(Memory* memory, Register* registers) {
	return decrement_opcode(memory, registers, &registers->bc.high);
}

static uint8_t opcode_0x06(Memory* memory, Register* registers)
{
	uint8_t high_byte = fetch_byte(memory, &registers->program_counter);
	registers->bc.high = high_byte;

	return 8;
}

static uint8_t opcode_0x07(Memory* memory, Register* registers)
{
	uint8_t bit7 = (registers->af.high >> 7) & 1;
	registers->af.high = (registers->af.high << 1) | bit7;

	set_register_flag(registers, Z, false);
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, false);
	set_register_flag(registers, C, bit7);

	return 4;
}

static uint8_t opcode_0x08(Memory* memory, Register* registers)
{
	memory16 address = fetch_two_bytes(memory, &registers->program_counter);
	memory16 stack_pointer;
	stack_pointer.value = registers->stack_pointer;

	memory_write(memory, address.value, stack_pointer.low);
	memory_write(memory, address.value + 1, stack_pointer.high);

	return 20;
}

static uint8_t opcode_0x09(Register* registers)
{
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, ((registers->hl.value & 0x0FFF) + (registers->bc.value & 0x0FFF)) > 0x0FFF);
	set_register_flag(registers, C, ((uint32_t)registers->hl.value + (uint32_t)registers->bc.value) > 0xFFFF);

	registers->hl.value += registers->bc.value;

	return 8;
}

static uint8_t opcode_0x0A(Memory* memory, Register* registers)
{
	registers->af.high = memory_read(memory, registers->bc.value);

	return 8;
}

static uint8_t opcode_0x0B(Register* registers)
{
	registers->bc.value--;

	return 8;
}

static uint8_t opcode_0x0C(Memory* memory, Register* registers) {
	return increment_opcode(memory, registers, &registers->bc.low);
}

static uint8_t opcode_0x0D(Memory* memory, Register* registers) {
	return decrement_opcode(memory, registers, &registers->bc.low);
}

static uint8_t opcode_0x0E(Memory* memory, Register* registers)
{
	registers->bc.low = fetch_byte(memory, &registers->program_counter);

	return 8;
}

static uint8_t opcode_0x0F(Register* registers)
{
	uint8_t bit0 = registers->af.high & 0x01;
	registers->af.high = (registers->af.high >> 1) | (bit0 << 7);

	set_register_flag(registers, Z, false);
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, false);
	set_register_flag(registers, C, bit0);

	return 4;
}

static uint8_t opcode_0x11(Memory* memory, Register* registers)
{
	registers->de.value = fetch_two_bytes(memory, &registers->program_counter).value;
	return 12;
}

static uint8_t opcode_0x12(Memory* memory, Register* registers)
{
	memory_write(memory, registers->de.value, registers->af.high);

	return 8;
}

static uint8_t opcode_0x13(Register* registers)
{
	registers->de.value++;

	return 8;
}

static uint8_t opcode_0x14(Memory* memory, Register* registers) {
	return increment_opcode(memory, registers, &registers->de.high);
}

static uint8_t opcode_0x15(Memory* memory, Register* registers) {
	return decrement_opcode(memory, registers, &registers->de.high);
}

static uint8_t opcode_0x16(Memory* memory, Register* registers)
{
	registers->de.high = fetch_byte(memory, &registers->program_counter);

	return 8;
}

static uint8_t opcode_0x17(Memory* memory, Register* registers)
{
	uint8_t old_carry = get_register_flag(memory, C);
	uint8_t bit7 = (registers->af.high >> 7) & 1;

	registers->af.high = (registers->af.high << 1) | old_carry;

	set_register_flag(memory, Z, false);
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, false);
	set_register_flag(memory, C, bit7);

	return 4;
}

static uint8_t opcode_0x18(Memory* memory, Register* registers)
{
	int8_t jump_value = (int8_t)fetch_byte(memory, &registers->program_counter);
	registers->program_counter.value += jump_value;

	return 12;
}

static uint8_t opcode_0x19(Memory* memory, Register* registers)
{
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, ((registers->hl.value & 0x0FFF) + (registers->de.value & 0x0FFF)) > 0x0FFF);
	set_register_flag(memory, C, ((uint32_t)registers->hl.value + (uint32_t)registers->de.value) > 0xFFFF);

	registers->hl.value += registers->de.value;

	return 8;
}

static uint8_t opcode_0x1A(Memory* memory, Register* registers)
{
	registers->af.high = memory_read(memory, registers->de.value);

	return 8;
}

static uint8_t opcode_0x1B(Memory* memory, Register* registers)
{
	registers->de.value--;

	return 8;
}

static uint8_t opcode_0x1C(Memory* memory, Register* registers) {
	return increment_opcode(memory, registers, &registers->de.low);
}

static uint8_t opcode_0x1D(Memory* memory, Register* registers) {
	return decrement_opcode(memory, registers, &registers->de.low);
}

static uint8_t opcode_0x1E(Memory* memory, Register* registers)
{
	registers->de.low = fetch_byte(memory, &registers->program_counter);

	return 8;
}

static uint8_t opcode_0x1F(Memory* memory, Register* registers)
{
	uint8_t old_carry = get_register_flag(memory, C);
	uint8_t bit0 = registers->af.high & 0x01;

	registers->af.high = (registers->af.high >> 1) | (old_carry << 7);

	set_register_flag(registers, Z, false);
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, false);
	set_register_flag(registers, C, bit0);

	return 4;
}

static uint8_t opcode_0x20(Memory* memory, Register* registers) {
	return conditional_jump_relative_opcode(memory, registers, Z, false);
}

static uint8_t opcode_0x21(Memory* memory, Register* registers)
{
	registers->hl.value = fetch_two_bytes(memory, &registers->program_counter).value;

	return 12;
}

static uint8_t opcode_0x22(Memory* memory, Register* registers)
{
	memory_write(memory, registers->hl.value, registers->af.high);
	registers->hl.value++;

	return 8;
}

static uint8_t opcode_0x23(Memory* memory, Register* registers)
{
	registers->hl.value++;

	return 8;
}

static uint8_t opcode_0x24(Memory* memory, Register* registers) {
	return increment_opcode(memory, registers, &registers->hl.high);
}

static uint8_t opcode_0x25(Memory* memory, Register* registers) {
	return decrement_opcode(memory, registers, &registers->hl.high);
}

static uint8_t opcode_0x26(Memory* memory, Register* registers)
{
	registers->hl.high = fetch_byte(memory, &registers->program_counter);

	return 8;
}

static uint8_t opcode_0x27(Memory* memory, Register* registers)
{
	uint8_t offset = 0;
	bool carry = false;

	uint8_t a_value = registers->af.high;
	bool h_flag = get_register_flag(registers, H);
	bool c_flag = get_register_flag(registers, C);
	bool n_flag = get_register_flag(registers, N);

	uint8_t low_byte_offset = 0x06;
	uint8_t high_byte_offset = 0x60;

	if (n_flag == false && (a_value & 0xF) > 0x09 || h_flag == true) {
		offset |= low_byte_offset;
	}

	if (n_flag == false && a_value > 0x99 || c_flag == true)
	{
		offset |= high_byte_offset;
		carry = true;
	}

	if (n_flag) {
		a_value -= offset;
	}
	else {
		a_value += offset;
	}

	registers->af.high = a_value;
	set_register_flag(registers, H, false);
	set_register_flag(registers, C, carry);
	set_register_flag(registers, Z, registers->af.high == 0);

	return 4;
}

static uint8_t opcode_0x28(Memory* memory, Register* registers) {
	return conditional_jump_relative_opcode(memory, registers, Z, true);
}

static uint8_t opcode_0x29(Memory* memory, Register* registers)
{
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, ((registers->hl.value & 0x0FFF) + (registers->hl.value & 0x0FFF)) > 0x0FFF);
	set_register_flag(registers, C, ((uint32_t)registers->hl.value + (uint32_t)registers->hl.value) > 0xFFFF);

	registers->hl.value += registers->hl.value;

	return 8;
}

static uint8_t opcode_0x2A(Memory* memory, Register* registers)
{
	registers->af.high = memory_read(memory, registers->hl.value);
	registers->hl.value++;

	return 8;
}

static uint8_t opcode_0x2B(Register* registers)
{
	registers->hl.value--;

	return 8;
}

static uint8_t opcode_0x2C(Memory* memory, Register* registers) {
	return increment_opcode(memory, registers, &registers->hl.low);
}

static uint8_t opcode_0x2D(Memory* memory, Register* registers) {
	return decrement_opcode(memory, registers, &registers->hl.low);
}

static uint8_t opcode_0x2E(Memory* memory, Register* registers)
{
	registers->hl.low = fetch_byte(memory, &registers->program_counter);

	return 8;
}

static uint8_t opcode_0x2F(Memory* memory, Register* registers)
{
	set_register_flag(registers, N, true);
	set_register_flag(registers, H, true);

	registers->af.high = ~registers->af.high;

	return 4;
}

static uint8_t opcode_0x30(Memory* memory, Register* registers) {
	return conditional_jump_relative_opcode(memory, registers, C, false);
}

static uint8_t opcode_0x31(Memory* memory, Register* registers)
{
	registers->stack_pointer = fetch_two_bytes(memory, &registers->program_counter).value;

	return 12;
}

static uint8_t opcode_0x32(Memory* memory, Register* registers)
{
	memory_write(memory, registers->hl.value, registers->af.high);
	registers->hl.value--;

	return 8;
}

static uint8_t opcode_0x33(Register* registers)
{
	registers->stack_pointer++;

	return 8;
}

static uint8_t opcode_0x34(Memory* memory, Register* registers)
{

	uint8_t value = memory_read(memory, registers->hl.value);

	increment_opcode(memory, registers, &value);
	memory_write(memory, registers->hl.value, value);

	return 12;
}

static uint8_t opcode_0x35(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	decrement_opcode(memory, registers, &value);
	memory_write(memory, registers->hl.value, value);

	return 12;
}

static uint8_t opcode_0x36(Memory* memory, Register* registers)
{
	memory_write(memory, registers->hl.value,
		fetch_byte(memory, &registers->program_counter));

	return 8;
}

static uint8_t opcode_0x37(Register* registers)
{
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, false);
	set_register_flag(registers, C, true);

	return 8;
}

static uint8_t opcode_0x38(Memory* memory, Register* registers) {
	return conditional_jump_relative_opcode(memory, registers, C, true);
}

static uint8_t opcode_0x39(Register* registers)
{
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, ((registers->hl.value & 0x0FFF) + (registers->stack_pointer & 0x0FFF)) > 0x0FFF);
	set_register_flag(registers, C, ((uint32_t)registers->hl.value + (uint32_t)registers->stack_pointer) > 0xFFFF);

	registers->hl.value += registers->stack_pointer;

	return 8;
}

static uint8_t opcode_0x3A(Memory* memory, Register* registers)
{
	registers->af.high = memory_read(memory, registers->hl.value);
	registers->hl.value--;

	return 8;
}

static uint8_t opcode_0x3B(Register* registers)
{
	registers->stack_pointer--;

	return 8;
}

static uint8_t opcode_0x3C(Memory* memory, Register* registers) {
	return increment_opcode(memory, registers, &registers->af.high);
}

static uint8_t opcode_0x3D(Memory* memory, Register* registers) {
	return decrement_opcode(memory, registers, &registers->af.high);
}

static uint8_t opcode_0x3E(Memory* memory, Register* registers)
{
	registers->af.high = fetch_byte(memory, &registers->program_counter);

	return 8;
}

static uint8_t opcode_0x3F(Register* registers)
{
	bool c_flag = get_register_flag(registers, C);

	set_register_flag(registers, N, false);
	set_register_flag(registers, H, false);
	set_register_flag(registers, C, !c_flag);

	return 4;
}

static uint8_t opcode_0x40(Register* registers)
{
	registers->bc.high = registers->bc.high;

	return 4;
}

static uint8_t opcode_0x41(Register* registers)
{
	registers->bc.high = registers->bc.low;

	return 4;
}

static uint8_t opcode_0x42(Register* registers)
{
	registers->bc.high = registers->de.high;

	return 4;
}

static uint8_t opcode_0x43(Register* registers)
{
	registers->bc.high = registers->de.low;

	return 4;
}

static uint8_t opcode_0x44(Register* registers)
{
	registers->bc.high = registers->hl.high;

	return 4;
}

static uint8_t opcode_0x45(Register* registers)
{
	registers->bc.high = registers->hl.low;

	return 4;
}

static uint8_t opcode_0x46(Memory* memory, Register* registers)
{
	registers->bc.high = memory_read(memory, registers->hl.value);

	return 8;
}

static uint8_t opcode_0x47(Register* registers)
{
	registers->bc.high = registers->af.high;

	return 4;
}

static uint8_t opcode_0x48(Register* registers)
{
	registers->bc.low = registers->bc.high;

	return 4;
}

static uint8_t opcode_0x49(Register* registers)
{
	registers->bc.low = registers->bc.low;

	return 4;
}

static uint8_t opcode_0x4A(Register* registers)
{
	registers->bc.low = registers->de.high;

	return 4;
}

static uint8_t opcode_0x4B(Register* registers)
{
	registers->bc.low = registers->de.low;

	return 4;
}

static uint8_t opcode_0x4C(Register* registers)
{
	registers->bc.low = registers->hl.high;

	return 4;
}

static uint8_t opcode_0x4D(Register* registers)
{
	registers->bc.low = registers->hl.low;

	return 4;
}

static uint8_t opcode_0x4E(Memory* memory, Register* registers)
{
	registers->bc.low = memory_read(memory, registers->hl.value);

	return 8;
}

static uint8_t opcode_0x4F(Register* registers)
{
	registers->bc.low = registers->af.high;

	return 4;
}

static uint8_t opcode_0x50(Register* registers)
{
	registers->de.high = registers->bc.high;

	return 4;
}

static uint8_t opcode_0x51(Register* registers)
{
	registers->de.high = registers->bc.low;

	return 4;
}

static uint8_t opcode_0x52(Register* registers)
{
	registers->de.high = registers->de.high;

	return 4;
}

static uint8_t opcode_0x53(Register* registers)
{
	registers->de.high = registers->de.low;

	return 4;
}

static uint8_t opcode_0x54(Register* registers)
{
	registers->de.high = registers->hl.high;

	return 4;
}

static uint8_t opcode_0x55(Register* registers)
{
	registers->de.high = registers->hl.low;

	return 4;
}

static uint8_t opcode_0x56(Memory* memory, Register* registers)
{
	registers->de.high = memory_read(memory, registers->hl.value);

	return 8;
}

static uint8_t opcode_0x57(Register* registers)
{
	registers->de.high = registers->af.high;

	return 4;
}

static uint8_t opcode_0x58(Register* registers)
{
	registers->de.low = registers->bc.high;

	return 4;
}

static uint8_t opcode_0x59(Register* registers)
{
	registers->de.low = registers->bc.low;

	return 4;
}

static uint8_t opcode_0x5A(Register* registers)
{
	registers->de.low = registers->de.high;

	return 4;
}

static uint8_t opcode_0x5B(Register* registers)
{
	registers->de.low = registers->de.low;

	return 4;
}

static uint8_t opcode_0x5C(Register* registers)
{
	registers->de.low = registers->hl.high;

	return 4;
}

static uint8_t opcode_0x5D(Register* registers)
{
	registers->de.low = registers->hl.low;

	return 4;
}

static uint8_t opcode_0x5E(Memory* memory, Register* registers)
{
	registers->de.low = memory_read(memory, registers->hl.value);

	return 8;
}

static uint8_t opcode_0x5F(Register* registers)
{
	registers->de.low = registers->af.high;

	return 4;
}

static uint8_t opcode_0x60(Register* registers)
{
	registers->hl.high = registers->bc.high;

	return 4;
}

static uint8_t opcode_0x61(Register* registers)
{
	registers->hl.high = registers->bc.low;

	return 4;
}

static uint8_t opcode_0x62(Register* registers)
{
	registers->hl.high = registers->de.high;

	return 4;
}

static uint8_t opcode_0x63(Register* registers)
{
	registers->hl.high = registers->de.low;

	return 4;
}

static uint8_t opcode_0x64(Register* registers)
{
	registers->hl.high = registers->hl.high;

	return 4;
}

static uint8_t opcode_0x65(Register* registers)
{
	registers->hl.high = registers->hl.low;

	return 4;
}

static uint8_t opcode_0x66(Memory* memory, Register* registers)
{
	registers->hl.high = memory_read(memory, registers->hl.value);

	return 8;
}

static uint8_t opcode_0x67(Register* registers)
{
	registers->hl.high = registers->af.high;

	return 4;
}

static uint8_t opcode_0x68(Register* registers)
{

	registers->hl.low = registers->bc.high;

	return 4;
}

static uint8_t opcode_0x69(Register* registers)
{
	registers->hl.low = registers->bc.low;

	return 4;
}

static uint8_t opcode_0x6A(Register* registers)
{
	registers->hl.low = registers->de.high;

	return 4;
}

static uint8_t opcode_0x6B(Register* registers)
{
	registers->hl.low = registers->de.low;

	return 4;
}

static uint8_t opcode_0x6C(Register* registers)
{
	registers->hl.low = registers->hl.high;

	return 4;
}

static uint8_t opcode_0x6D(Register* registers)
{
	registers->hl.low = registers->hl.low;

	return 4;
}

static uint8_t opcode_0x6E(Memory* memory, Register* registers)
{
	registers->hl.low = memory_read(memory, registers->hl.value);

	return 8;
}

static uint8_t opcode_0x6F(Register* registers)
{
	registers->hl.low = registers->af.high;

	return 4;
}

static uint8_t opcode_0x70(Memory* memory, Register* registers)
{
	memory_write(memory, registers->hl.value, registers->bc.high);

	return 8;
}

static uint8_t opcode_0x71(Memory* memory, Register* registers)
{
	memory_write(memory, registers->hl.value, registers->bc.low);

	return 8;
}

static uint8_t opcode_0x72(Memory* memory, Register* registers)
{
	memory_write(memory, registers->hl.value, registers->de.high);

	return 8;
}

static uint8_t opcode_0x73(Memory* memory, Register* registers)
{
	memory_write(memory, registers->hl.value, registers->de.low);

	return 8;
}

static uint8_t opcode_0x74(Memory* memory, Register* registers)
{
	memory_write(memory, registers->hl.value, registers->hl.high);

	return 8;
}

static uint8_t opcode_0x75(Memory* memory, Register* registers)
{
	memory_write(memory, registers->hl.value, registers->hl.low);

	return 8;
}

static uint8_t opcode_0x76()
{
	is_halted = true;

	return 4;
}

static uint8_t opcode_0x77(Memory* memory, Register* registers)
{
	memory_write(memory, registers->hl.value, registers->af.high);

	return 8;
}

static uint8_t opcode_0x78(Register* registers)
{
	registers->af.high = registers->bc.high;

	return 4;
}

static uint8_t opcode_0x79(Register* registers)
{
	registers->af.high = registers->bc.low;

	return 4;
}

static uint8_t opcode_0x7A(Register* registers)
{
	registers->af.high = registers->de.high;

	return 4;
}

static uint8_t opcode_0x7B(Register* registers)
{
	registers->af.high = registers->de.low;

	return 4;
}

static uint8_t opcode_0x7C(Register* registers)
{
	registers->af.high = registers->hl.high;

	return 4;
}

static uint8_t opcode_0x7D(Register* registers)
{
	registers->af.high = registers->hl.low;

	return 4;
}

static uint8_t opcode_0x7E(Memory* memory, Register* registers)
{
	registers->af.high = memory_read(memory, registers->hl.value);

	return 8;
}


static uint8_t opcode_0x7F(Memory* memory, Register* registers)
{
	registers->af.high = registers->af.high;

	return 4;
}

static uint8_t opcode_0x80(Register* registers) {
	return add_opcode(registers, registers->bc.high);
}

static uint8_t opcode_0x81(Register* registers) {
	return add_opcode(registers, registers->bc.low);
}

static uint8_t opcode_0x82(Register* registers) {
	return add_opcode(registers, registers->de.high);
}

static uint8_t opcode_0x83(Register* registers) {
	return add_opcode(registers, registers->de.low);
}

static uint8_t opcode_0x84(Register* registers) {
	return add_opcode(registers, registers->hl.high);
}

static uint8_t opcode_0x85(Register* registers) {
	return add_opcode(registers, registers->hl.low);
}

static uint8_t opcode_0x86(Memory* memory, Register* registers)
{
	uint8_t memory_value = memory_read(memory, registers->hl.value);
	add_opcode(memory, memory_value);

	return 8;
}

static uint8_t opcode_0x87(Register* registers) {
	return add_opcode(registers, registers->af.high);
}

static uint8_t opcode_0x88(Register* registers) {
	return add_c_opcode(registers, registers->bc.high);
}

static uint8_t opcode_0x89(Register* registers) {
	return add_c_opcode(registers, registers->bc.low);
}

static uint8_t opcode_0x8A(Register* registers) {
	return add_c_opcode(registers, registers->de.high);
}

static uint8_t opcode_0x8B(Register* registers) {
	return add_c_opcode(registers, registers->de.low);
}

static uint8_t opcode_0x8C(Register* registers) {
	return add_c_opcode(registers, registers->hl.high);
}

static uint8_t opcode_0x8D(Register* registers) {
	return add_c_opcode(registers, registers->hl.low);
}

static uint8_t opcode_0x8E(Memory* memory, Register* registers)
{
	uint8_t memory_value = memory_read(memory, registers->hl.value);
	add_c_opcode(registers, memory_value);
	return 8;
}

static uint8_t opcode_0x8F(Register* registers) {
	return add_c_opcode(registers, registers->af.high);
}

static uint8_t opcode_0x90(Register* registers) {
	return subtract_opcode(registers, registers->bc.high);
}

static uint8_t opcode_0x91(Register* registers) {
	return subtract_opcode(registers, registers->bc.low);
}

static uint8_t opcode_0x92(Register* registers) {
	return subtract_opcode(registers, registers->de.high);
}

static uint8_t opcode_0x93(Register* registers) {
	return subtract_opcode(registers, registers->de.low);
}

static uint8_t opcode_0x94(Register* registers) {
	return subtract_opcode(registers, registers->hl.high);
}

static uint8_t opcode_0x95(Register* registers) {
	return subtract_opcode(registers, registers->hl.low);
}

static uint8_t opcode_0x96(Memory* memory, Register* registers)
{
	uint8_t memory_value = memory_read(memory, registers->hl.value);
	subtract_opcode(memory, memory_value);

	return 8;
}

static uint8_t opcode_0x97(Register* registers) {
	return subtract_opcode(registers, registers->af.high);
}

static uint8_t opcode_0x98(Register* registers) {
	return subtract_c_opcode(registers, registers->bc.high);
}

static uint8_t opcode_0x99(Register* registers) {
	return subtract_c_opcode(registers, registers->bc.low);
}

static uint8_t opcode_0x9A(Register* registers) {
	return subtract_c_opcode(registers, registers->de.high);
}

static uint8_t opcode_0x9B(Register* registers) {
	return subtract_c_opcode(registers, registers->de.low);
}

static uint8_t opcode_0x9C(Register* registers) {
	return subtract_c_opcode(registers, registers->hl.high);
}

static uint8_t opcode_0x9D(Register* registers) {
	return subtract_c_opcode(registers, registers->hl.low);
}

static uint8_t opcode_0x9E(Memory* memory, Register* registers)
{
	uint8_t memory_value = memory_read(memory, registers->hl.value);
	subtract_c_opcode(memory, memory_value);

	return 8;
}

static uint8_t opcode_0x9F(Register* registers) {
	return subtract_c_opcode(registers, registers->af.high);
}

static uint8_t opcode_0xA0(Register* registers) {
	return and_opcode(registers, registers->bc.high);
}

static uint8_t opcode_0xA1(Register* registers) {
	return and_opcode(registers, registers->bc.low);
}

static uint8_t opcode_0xA2(Register* registers) {
	return and_opcode(registers, registers->de.high);
}

static uint8_t opcode_0xA3(Register* registers) {
	return and_opcode(registers, registers->de.low);
}

static uint8_t opcode_0xA4(Register* registers) {
	return and_opcode(registers, registers->hl.high);
}

static uint8_t opcode_0xA5(Register* registers) {
	return and_opcode(registers, registers->hl.low);
}

static uint8_t opcode_0xA6(Memory* memory, Register* registers)
{
	and_opcode(registers, memory_read(memory, registers->hl.value));

	return 8;
}

static uint8_t opcode_0xA7(Register* registers) {
	return and_opcode(registers, registers->af.high);
}

static uint8_t opcode_0xA8(Register* registers) {
	return xor_opcode(registers, registers->bc.high);
}

static uint8_t opcode_0xA9(Register* registers) {
	return xor_opcode(registers, registers->bc.low);
}

static uint8_t opcode_0xAA(Register* registers) {
	return xor_opcode(registers, registers->de.high);
}

static uint8_t opcode_0xAB(Register* registers) {
	return xor_opcode(registers, registers->de.low);
}

static uint8_t opcode_0xAC(Register* registers) {
	return xor_opcode(registers, registers->hl.high);
}

static uint8_t opcode_0xAD(Register* registers) {
	return xor_opcode(registers, registers->hl.low);
}

static uint8_t opcode_0xAE(Memory* memory, Register* registers)
{
	xor_opcode(registers, memory_read(memory, registers->hl.value));

	return 8;
}

static uint8_t opcode_0xAF(Register* registers) {
	return xor_opcode(registers, registers->af.high);
}

static uint8_t opcode_0xB0(Register* registers) {
	return or_opcode(registers, registers->bc.high);
}

static uint8_t opcode_0xB1(Register* registers) {
	return or_opcode(registers, registers->bc.low);
}

static uint8_t opcode_0xB2(Register* registers) {
	return or_opcode(registers, registers->de.high);
}

static uint8_t opcode_0xB3(Register* registers) {
	return or_opcode(registers, registers->de.low);
}

static uint8_t opcode_0xB4(Register* registers) {
	return or_opcode(registers, registers->hl.high);
}

static uint8_t opcode_0xB5(Register* registers) {
	return or_opcode(registers, registers->hl.low);
}

static uint8_t opcode_0xB6(Memory* memory, Register* registers)
{
	or_opcode(memory, memory_read(memory, registers->hl.value));

	return 8;
}

static uint8_t opcode_0xB7(Register* registers) {
	return or_opcode(registers, registers->af.high);
}

static uint8_t opcode_0xB8(Register* registers) {
	return compare_opcode(registers, registers->bc.high);
}

static uint8_t opcode_0xB9(Register* registers) {
	return compare_opcode(registers, registers->bc.low);
}

static uint8_t opcode_0xBA(Register* registers) {
	return compare_opcode(registers, registers->de.high);
}

static uint8_t opcode_0xBB(Register* registers) {
	return compare_opcode(registers, registers->de.low);
}

static uint8_t opcode_0xBC(Register* registers) {
	return compare_opcode(registers, registers->hl.high);
}

static uint8_t opcode_0xBD(Register* registers) {
	return compare_opcode(registers, registers->hl.low);
}

static uint8_t opcode_0xBE(Memory* memory, Register* registers)
{
	compare_opcode(memory, memory_read(memory, registers->hl.value));

	return 8;
}

static uint8_t opcode_0xBF(Register* registers) {
	return compare_opcode(registers, registers->af.high);
}

static uint8_t opcode_0xC0(Memory* memory, Register* registers) {
	return conditional_return_opcode(memory, registers, Z, false);
}

static uint8_t opcode_0xC1(Memory* memory, Register* registers)
{
	registers->bc.value = fetch_two_bytes(memory, &registers->stack_pointer).value;

	return 12;
}

static uint8_t opcode_0xC2(Memory* memory, Register* registers) {
	return conditional_jump_absolute_opcode(memory, registers, Z, false);
}

static uint8_t opcode_0xC3(Memory* memory, Register* registers)
{
	registers->program_counter = fetch_two_bytes(memory, &registers->program_counter);

	return 16;
}

static uint8_t opcode_0xC4(Memory* memory, Register* registers) {
	return conditional_call_opcode(memory, registers, Z, false);
}

static uint8_t opcode_0xC5(Memory* memory, Register* registers)
{
	write_byte(memory, &registers->stack_pointer, registers->bc.high);
	write_byte(memory, &registers->stack_pointer, registers->bc.low);

	return 16;
}

static uint8_t opcode_0xC6(Memory* memory, Register* registers)
{
	uint8_t byte_value = fetch_byte(memory, &registers->program_counter);
	add_opcode(memory, byte_value);

	return 8;
}

static uint8_t opcode_0xC7(Memory* memory, Register* registers) {
	return restart_opcode(memory, registers, 0x00);
}

static uint8_t opcode_0xC8(Memory* memory, Register* registers) {
	return conditional_return_opcode(memory, registers, Z, true);
}

static uint8_t opcode_0xC9(Memory* memory, Register* registers)
{
	uint16_t memory_value = fetch_two_bytes(memory, &registers->stack_pointer).value;
	registers->program_counter.value = memory_value;

	return 16;
}

static uint8_t opcode_0xCA(Memory* memory, Register* registers) {
	return conditional_jump_absolute_opcode(memory, registers, Z, true);
}

static uint8_t opcode_0xCC(Memory* memory, Register* registers) {
	return conditional_call_opcode(memory, registers, Z, true);
}

static uint8_t opcode_0xCD(Memory* memory, Register* registers)
{
	memory16 jump_target = fetch_two_bytes(memory, &registers->program_counter);

	return call_opcode(memory, registers, jump_target);
}

static uint8_t opcode_0xCE(Memory* memory, Register* registers)
{
	add_c_opcode(memory, fetch_byte(memory, &registers->program_counter.value));

	return 8;
}

static uint8_t opcode_0xCF(Memory* memory, Register* registers) {
	return restart_opcode(memory, registers, 0x08);
}

static uint8_t opcode_0xD0(Memory* memory, Register* registers) {
	return conditional_return_opcode(memory, registers, C, false);
}

static uint8_t opcode_0xD1(Memory* memory, Register* registers)
{
	registers->de.value = fetch_two_bytes(memory, &registers->stack_pointer).value;

	return 12;
}

static uint8_t opcode_0xD2(Memory* memory, Register* registers) {
	return conditional_jump_absolute_opcode(memory, registers,C, false);
}

static uint8_t opcode_0xD4(Memory* memory, Register* registers) {
	return conditional_call_opcode(memory, registers, C, false);
}

static uint8_t opcode_0xD5(Memory* memory, Register* registers)
{
	write_byte(memory, &registers->stack_pointer, registers->de.high);
	write_byte(memory, &registers->stack_pointer, registers->de.low);

	return 16;
}

static uint8_t opcode_0xD6(Memory* memory, Register* registers)
{
	uint8_t memory_value = fetch_byte(memory, &registers->program_counter);
	subtract_opcode(memory, memory_value);

	return 8;
}

static uint8_t opcode_0xD7(Memory* memory, Register* registers) {
	return restart_opcode(memory, registers, 0x10);
}

static uint8_t opcode_0xD8(Memory* memory, Register* registers)
{
	bool c_flag = get_register_flag(registers, C);

	if (c_flag)
	{
		uint16_t memory_value = fetch_two_bytes(memory, &registers->stack_pointer).value;
		registers->program_counter.value = memory_value;

		return 20;
	}

	return 8;
}

static uint8_t opcode_0xD9(Memory* memory, Register* registers)
{
	memory16 return_address;
	return_address.low = fetch_byte(memory, &registers->stack_pointer);
	return_address.high = fetch_byte(memory, &registers->stack_pointer);

	registers->program_counter.value = return_address.value;
	interrupt_master_enable = true;

	return 16;
}

static uint8_t opcode_0xDA(Memory* memory, Register* registers) {
	return conditional_jump_absolute_opcode(memory, registers, C, true);
}

static uint8_t opcode_0xDC(Memory* memory, Register* registers) {
	return conditional_call_opcode(memory, registers, C, true);
}

static uint8_t opcode_0xDE(Memory* memory, Register* registers)
{
	subtract_c_opcode(memory, fetch_byte(memory, &registers->program_counter));

	return 8;
}

static uint8_t opcode_0xDF(Memory* memory, Register* registers) {
	return restart_opcode(memory, registers, 0x18);
}

static uint8_t opcode_0xE0(Memory* memory, Register* registers)
{
	uint8_t memory_value = fetch_byte(memory, &registers->program_counter);

	memory16 memory_address;
	memory_address.high = 0xFF;
	memory_address.low = memory_value;

	memory_write(memory, memory_address.value, registers->af.high);

	return 12;
}

static uint8_t opcode_0xE1(Memory* memory, Register* registers)
{
	registers->hl = fetch_two_bytes(memory, &registers->stack_pointer);

	return 12;
}

static uint8_t opcode_0xE2(Memory* memory, Register* registers)
{
	memory16 memory_address;
	memory_address.high = 0xFF;
	memory_address.low = registers->bc.low;

	memory_write(memory, memory_address.value, registers->af.high);

	return 8;
}

static uint8_t opcode_0xE5(Memory* memory, Register* registers)
{
	write_byte(memory, &registers->stack_pointer, registers->hl.high);
	write_byte(memory, &registers->stack_pointer, registers->hl.low);

	return 16;
}

static uint8_t opcode_0xE6(Memory* memory, Register* registers)
{
	and_opcode(memory, fetch_byte(memory, &registers->program_counter));

	return 8;
}

static uint8_t opcode_0xE7(Memory* memory, Register* registers) {
	return restart_opcode(memory, registers, 0x20);
}

static uint8_t opcode_0xE8(Memory* memory, Register* registers)
{
	uint8_t raw_byte = fetch_byte(memory, &registers->program_counter);
	int8_t signed_offset = (int8_t)raw_byte;

	set_register_flag(memory, Z, false);
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, ((registers->stack_pointer & 0x0F) + (raw_byte & 0x0F)) > 0x0F);
	set_register_flag(memory, C, ((registers->stack_pointer & 0xFF) + raw_byte) > 0xFF);

	registers->stack_pointer += signed_offset;

	return 16;
}

static uint8_t opcode_0xE9(Memory* memory, Register* registers)
{
	registers->program_counter.value = registers->hl.value;

	return 4;
}

static uint8_t opcode_0xEA(Memory* memory, Register* registers)
{
	memory16 memory_address = fetch_two_bytes(memory, &registers->program_counter);
	memory_write(memory, memory_address.value, registers->af.high);

	return 16;
}

static uint8_t opcode_0xEE(Memory* memory, Register* registers)
{
	xor_opcode(memory, fetch_byte(memory, &registers->program_counter));

	return 8;
}

static uint8_t opcode_0xEF(Memory* memory, Register* registers) {
	return restart_opcode(memory, registers, 0x28);
}

static uint8_t opcode_0xF0(Memory* memory, Register* registers)
{
	uint8_t memory_value = fetch_byte(memory, &registers->program_counter);

	memory16 address;
	address.high = 0xFF;
	address.low = memory_value;

	registers->af.high = memory_read(memory, address.value);

	return 12;
}

static uint8_t opcode_0xF1(Memory* memory, Register* registers)
{
	registers->af = fetch_two_bytes(memory, &registers->stack_pointer);
	registers->af.low &= 0xF0;

	return 12;
}

static uint8_t opcode_0xF2(Memory* memory, Register* registers)
{
	memory16 address;
	address.high = 0xFF;
	address.low = registers->bc.low;

	registers->af.high = memory_read(memory, address.value);

	return 8;
}

static uint8_t opcode_0xF3()
{
	interrupt_master_enable = false;
	interrupt_enable_pending = false;

	return 4;
}

static uint8_t opcode_0xF5(Memory* memory, Register* registers)
{
	write_byte(memory, &registers->stack_pointer, registers->af.high);
	write_byte(memory, &registers->stack_pointer, registers->af.low);

	return 16;
}

static uint8_t opcode_0xF6(Memory* memory, Register* registers)
{
	or_opcode(memory, fetch_byte(memory, &registers->program_counter));

	return 8;
}

static uint8_t opcode_0xF7(Memory* memory, Register* registers) {
	return restart_opcode(memory, registers, 0x30);
}

static uint8_t opcode_0xF8(Memory* memory, Register* registers)
{
	uint8_t raw_byte = fetch_byte(memory, &registers->program_counter);
	int8_t signed_offset = (int8_t)raw_byte;

	set_register_flag(memory, Z, false);
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, ((registers->stack_pointer & 0x0F) + (raw_byte & 0x0F)) > 0x0F);
	set_register_flag(memory, C, ((registers->stack_pointer & 0xFF) + raw_byte) > 0xFF);

	registers->hl.value = signed_offset + registers->stack_pointer;

	return 12;
}

static uint8_t opcode_0xF9(Register* registers)
{
	registers->stack_pointer = registers->hl.value;

	return 8;
}

static uint8_t opcode_0xFA(Memory* memory, Register* registers)
{
	memory16 address = fetch_two_bytes(memory, &registers->program_counter);
	registers->af.high = memory_read(memory, address.value);

	return 16;
}

static uint8_t opcode_0xFB()
{
	interrupt_enable_pending = true;

	return 4;
}

static uint8_t opcode_0xFE(Memory* memory, Register* registers)
{
	compare_opcode(memory, fetch_byte(memory, &registers->program_counter));

	return 8;
}

static uint8_t opcode_0xFF(Memory* memory, Register* registers) {
	return restart_opcode(memory, registers, 0x38);
}

uint8_t opcode_step(Memory* memory, Register* registers, uint8_t opcode)
{
	if (interrupt_enable_pending)
	{
		interrupt_master_enable = true;
		interrupt_enable_pending = false;
	}

	if (is_halted)
	{
		if (is_pending(memory) != 0) {
			is_halted = false;
		}
		else {
			return 4;
		}
	}

	uint8_t cycles = 4;

	switch (opcode)
	{
		case 0x00:
			break;
		case 0x01:
			cycles = opcode_0x01(memory, registers);
			break;
		case 0x02:
			cycles = opcode_0x02(memory, registers);
			break;
		case 0x03:
			cycles = opcode_0x03(registers);
			break;
		case 0x04:
			cycles = opcode_0x04(memory, registers);
			break;
		case 0x05:
			cycles = opcode_0x05(memory, registers);
			break;
		case 0x06:
			cycles = opcode_0x06(memory, registers);
			break;
		case 0x07:
			cycles = opcode_0x07(memory, registers);
			break;
		case 0x08:
			cycles = opcode_0x08(memory, registers);
			break;
		case 0x09:
			cycles = opcode_0x09(registers);
			break;
		case 0x0A:
			cycles = opcode_0x0A(memory, registers);
			break;
		case 0x0B:
			cycles = opcode_0x0B(registers);
			break;
		case 0x0C:
			cycles = opcode_0x0C(memory, registers);
			break;
		case 0x0D:
			cycles = opcode_0x0D(memory, registers);
			break;
		case 0x0E:
			cycles = opcode_0x0E(memory, registers);
			break;
		case 0x0F:
			cycles = opcode_0x0F(registers);
			break;
		case 0x11:
			cycles = opcode_0x11(memory, registers);
			break;
		case 0x12:
			cycles = opcode_0x12(memory, registers);
			break;
		case 0x13:
			cycles = opcode_0x13(registers);
			break;
		case 0x14:
			cycles = opcode_0x14(memory, registers);
			break;
		case 0x15:
			cycles = opcode_0x15(memory, registers);
			break;
		case 0x16:
			cycles = opcode_0x16(memory, registers);
			break;
		case 0x17:
			cycles = opcode_0x17(memory, registers);
			break;
		case 0x18:
			cycles = opcode_0x18(memory, registers);
			break;
		case 0x19:
			cycles = opcode_0x19(memory, registers);
			break;
		case 0x1A:
			cycles = opcode_0x1A(memory, registers);
			break;
		case 0x1B:
			cycles = opcode_0x1B(memory, registers);
			break;
		case 0x1C:
			cycles = opcode_0x1C(memory, registers);
			break;
		case 0x1D:
			cycles = opcode_0x1D(memory, registers);
			break;
		case 0x1E:
			cycles = opcode_0x1E(memory, registers);
			break;
		case 0x1F:
			cycles = opcode_0x1F(memory, registers);
			break;
		case 0x20:
			cycles = opcode_0x20(memory, registers);
			break;
		case 0x21:
			cycles = opcode_0x21(memory, registers);
			break;
		case 0x22:
			cycles = opcode_0x22(memory, registers);
			break;
		case 0x23:
			cycles = opcode_0x23(memory, registers);
			break;
		case 0x24:
			cycles = opcode_0x24(memory, registers);
			break;
		case 0x25:
			cycles = opcode_0x25(memory, registers);
			break;
		case 0x26:
			cycles = opcode_0x26(memory, registers);
			break;
		case 0x27:
			cycles = opcode_0x27(memory, registers);
			break;
		case 0x28:
			cycles = opcode_0x28(memory, registers);
			break;
		case 0x29:
			cycles = opcode_0x29(memory, registers);
			break;
		case 0x2A:
			cycles = opcode_0x2A(memory, registers);
			break;
		case 0x2B:
			cycles = opcode_0x2B(registers);
			break;
		case 0x2C:
			cycles = opcode_0x2C(memory, registers);
			break;
		case 0x2D:
			cycles = opcode_0x2D(memory, registers);
			break;
		case 0x2E:
			cycles = opcode_0x2E(memory, registers);
			break;
		case 0x2F:
			cycles = opcode_0x2F(memory, registers);
			break;
		case 0x30:
			cycles = opcode_0x30(memory, registers);
			break;
		case 0x31:
			cycles = opcode_0x31(memory, registers);
			break;
		case 0x32:
			cycles = opcode_0x32(memory, registers);
			break;
		case 0x33:
			cycles = opcode_0x33(registers);
			break;
		case 0x34:
			cycles = opcode_0x34(memory, registers);
			break;
		case 0x35:
			cycles = opcode_0x35(memory, registers);
			break;
		case 0x36:
			cycles = opcode_0x36(memory, registers);
			break;
		case 0x37:
			cycles = opcode_0x37(registers);
			break;
		case 0x38:
			cycles = opcode_0x38(memory, registers);
			break;
		case 0x39:
			cycles = opcode_0x39(registers);
			break;
		case 0x3A:
			cycles = opcode_0x3A(memory, registers);
			break;
		case 0x3B:
			cycles = opcode_0x3B(registers);
			break;
		case 0x3C:
			cycles = opcode_0x3C(memory, registers);
			break;
		case 0x3D:
			cycles = opcode_0x3D(memory, registers);
			break;
		case 0x3E:
			cycles = opcode_0x3E(memory, registers);
			break;
		case 0x3F:
			cycles = opcode_0x3F(registers);
			break;
		case 0x40:
			cycles = opcode_0x40(registers);
			break;
		case 0x41:
			cycles = opcode_0x41(registers);
			break;
		case 0x42:
			cycles = opcode_0x42(registers);
			break;
		case 0x43:
			cycles = opcode_0x43(registers);
			break;
		case 0x44:
			cycles = opcode_0x44(registers);
			break;
		case 0x45:
			cycles = opcode_0x45(registers);
			break;
		case 0x46:
			cycles = opcode_0x46(memory, registers);
			break;
		case 0x47:
			cycles = opcode_0x47(registers);
			break;
		case 0x48:
			cycles = opcode_0x48(registers);
			break;
		case 0x49:
			cycles = opcode_0x49(registers);
			break;
		case 0x4A:
			cycles = opcode_0x4A(registers);
			break;
		case 0x4B:
			cycles = opcode_0x4B(registers);
			break;
		case 0x4C:
			cycles = opcode_0x4C(registers);
			break;
		case 0x4D:
			cycles = opcode_0x4D(registers);
			break;
		case 0x4E:
			cycles = opcode_0x4E(memory, registers);
			break;
		case 0x4F:
			cycles = opcode_0x4F(registers);
			break;
		case 0x50:
			cycles = opcode_0x50(registers);
			break;
		case 0x51:
			cycles = opcode_0x51(registers);
			break;
		case 0x52:
			cycles = opcode_0x52(registers);
			break;
		case 0x53:
			cycles = opcode_0x53(registers);
			break;
		case 0x54:
			cycles = opcode_0x54(registers);
			break;
		case 0x55:
			cycles = opcode_0x55(registers);
			break;
		case 0x56:
			cycles = opcode_0x56(memory, registers);
			break;
		case 0x57:
			cycles = opcode_0x57(registers);
			break;
		case 0x58:
			cycles = opcode_0x58(registers);
			break;
		case 0x59:
			cycles = opcode_0x59(registers);
			break;
		case 0x5A:
			cycles = opcode_0x5A(registers);
			break;
		case 0x5B:
			cycles = opcode_0x5B(registers);
			break;
		case 0x5C:
			cycles = opcode_0x5C(registers);
			break;
		case 0x5D:
			cycles = opcode_0x5D(registers);
			break;
		case 0x5E:
			cycles = opcode_0x5E(memory, registers);
			break;
		case 0x5F:
			cycles = opcode_0x5F(registers);
			break;
		case 0x60:
			cycles = opcode_0x60(registers);
			break;
		case 0x61:
			cycles = opcode_0x61(registers);
			break;
		case 0x62:
			cycles = opcode_0x62(registers);
			break;
		case 0x63:
			cycles = opcode_0x63(registers);
			break;
		case 0x64:
			cycles = opcode_0x64(registers);
			break;
		case 0x65:
			cycles = opcode_0x65(registers);
			break;
		case 0x66:
			cycles = opcode_0x66(memory, registers);
			break;
		case 0x67:
			cycles = opcode_0x67(registers);
			break;
		case 0x68:
			cycles = opcode_0x68(registers);
			break;
		case 0x69:
			cycles = opcode_0x69(registers);
			break;
		case 0x6A:
			cycles = opcode_0x6A(registers);
			break;
		case 0x6B:
			cycles = opcode_0x6B(registers);
			break;
		case 0x6C:
			cycles = opcode_0x6C(registers);
			break;
		case 0x6D:
			cycles = opcode_0x6D(registers);
			break;
		case 0x6E:
			cycles = opcode_0x6E(memory, registers);
			break;
		case 0x6F:
			cycles = opcode_0x6F(registers);
			break;
		case 0x70:
			cycles = opcode_0x70(memory, registers);
			break;
		case 0x71:
			cycles = opcode_0x71(memory, registers);
			break;
		case 0x72:
			cycles = opcode_0x72(memory, registers);
			break;
		case 0x73:
			cycles = opcode_0x73(memory, registers);
			break;
		case 0x74:
			cycles = opcode_0x74(memory, registers);
			break;
		case 0x75:
			cycles = opcode_0x75(memory, registers);
			break;
		case 0x76:
			cycles = opcode_0x76();
			break;
		case 0x77:
			cycles = opcode_0x77(memory, registers);
			break;
		case 0x78:
			cycles = opcode_0x78(registers);
			break;
		case 0x79:
			cycles = opcode_0x79(registers);
			break;
		case 0x7A:
			cycles = opcode_0x7A(registers);
			break;
		case 0x7B:
			cycles = opcode_0x7B(registers);
			break;
		case 0x7C:
			cycles = opcode_0x7C(registers);
			break;
		case 0x7D:
			cycles = opcode_0x7D(registers);
			break;
		case 0x7E:
			cycles = opcode_0x7E(memory, registers);
			break;
		case 0x7F:
			cycles = opcode_0x7F(memory, registers);
			break;
		case 0x80:
			cycles = opcode_0x80(registers);
			break;
		case 0x81:
			cycles = opcode_0x81(registers);
			break;
		case 0x82:
			cycles = opcode_0x82(registers);
			break;
		case 0x83:
			cycles = opcode_0x83(registers);
			break;
		case 0x84:
			cycles = opcode_0x84(registers);
			break;
		case 0x85:
			cycles = opcode_0x85(registers);
			break;
		case 0x86:
			cycles = opcode_0x86(memory, registers);
			break;
		case 0x87:
			cycles = opcode_0x87(registers);
			break;
		case 0x88:
			cycles = opcode_0x88(registers);
			break;
		case 0x89:
			cycles = opcode_0x89(registers);
			break;
		case 0x8A:
			cycles = opcode_0x8A(registers);
			break;
		case 0x8B:
			cycles = opcode_0x8B(registers);
			break;
		case 0x8C:
			cycles = opcode_0x8C(registers);
			break;
		case 0x8D:
			cycles = opcode_0x8D(registers);
			break;
		case 0x8E:
			cycles = opcode_0x8E(memory, registers);
			break;
		case 0x8F:
			cycles = opcode_0x8F(memory);
			break;
		case 0x90:
			cycles = opcode_0x90(registers);
			break;
		case 0x91:
			cycles = opcode_0x91(registers);
			break;
		case 0x92:
			cycles = opcode_0x92(registers);
			break;
		case 0x93:
			cycles = opcode_0x93(registers);
			break;
		case 0x94:
			cycles = opcode_0x94(registers);
			break;
		case 0x95:
			cycles = opcode_0x95(registers);
			break;
		case 0x96:
			cycles = opcode_0x96(memory, registers);
			break;
		case 0x97:
			cycles = opcode_0x97(registers);
			break;
		case 0x98:
			cycles = opcode_0x98(registers);
			break;
		case 0x99:
			cycles = opcode_0x99(registers);
			break;
		case 0x9A:
			cycles = opcode_0x9A(registers);
			break;
		case 0x9B:
			cycles = opcode_0x9B(registers);
			break;
		case 0x9C:
			cycles = opcode_0x9C(registers);
			break;
		case 0x9D:
			cycles = opcode_0x9D(registers);
			break;
		case 0x9E:
			cycles = opcode_0x9E(memory, registers);
			break;
		case 0x9F:
			cycles = opcode_0x9F(registers);
			break;
		case 0xA0:
			cycles = opcode_0xA0(registers);
			break;
		case 0xA1:
			cycles = opcode_0xA1(registers);
			break;
		case 0xA2:
			cycles = opcode_0xA2(registers);
			break;
		case 0xA3:
			cycles = opcode_0xA3(registers);
			break;
		case 0xA4:
			cycles = opcode_0xA4(registers);
			break;
		case 0xA5:
			cycles = opcode_0xA5(registers);
			break;
		case 0xA6:
			cycles = opcode_0xA6(memory, registers);
			break;
		case 0xA7:
			cycles = opcode_0xA7(registers);
			break;
		case 0xA8:
			cycles = opcode_0xA8(registers);
			break;
		case 0xA9:
			cycles = opcode_0xA9(registers);
			break;
		case 0xAA:
			cycles = opcode_0xAA(registers);
			break;
		case 0xAB:
			cycles = opcode_0xAB(registers);
			break;
		case 0xAC:
			cycles = opcode_0xAC(registers);
			break;
		case 0xAD:
			cycles = opcode_0xAD(registers);
			break;
		case 0xAE:
			cycles = opcode_0xAE(memory, registers);
			break;
		case 0xAF:
			cycles = opcode_0xAF(registers);
			break;
		case 0xB0:
			cycles = opcode_0xB0(registers);
			break;
		case 0xB1:
			cycles = opcode_0xB1(registers);
			break;
		case 0xB2:
			cycles = opcode_0xB2(registers);
			break;
		case 0xB3:
			cycles = opcode_0xB3(registers);
			break;
		case 0xB4:
			cycles = opcode_0xB4(registers);
			break;
		case 0xB5:
			cycles = opcode_0xB5(registers);
			break;
		case 0xB6:
			cycles = opcode_0xB6(memory, registers);
			break;
		case 0xB7:
			cycles = opcode_0xB7(registers);
			break;
		case 0xB8:
			cycles = opcode_0xB8(registers);
			break;
		case 0xB9:
			cycles = opcode_0xB9(registers);
			break;
		case 0xBA:
			cycles = opcode_0xBA(registers);
			break;
		case 0xBB:
			cycles = opcode_0xBB(registers);
			break;
		case 0xBC:
			cycles = opcode_0xBC(registers);
			break;
		case 0xBD:
			cycles = opcode_0xBD(registers);
			break;
		case 0xBE:
			cycles = opcode_0xBE(memory, registers);
			break;
		case 0xBF:
			cycles = opcode_0xBF(registers);
			break;
		case 0xC0:
			cycles = opcode_0xC0(memory, registers);
			break;
		case 0xC1:
			cycles = opcode_0xC1(memory, registers);
			break;
		case 0xC2:
			cycles = opcode_0xC2(memory, registers);
			break;
		case 0xC3:
			cycles = opcode_0xC3(memory, registers);
			break;
		case 0xC4:
			cycles = opcode_0xC4(memory, registers);
			break;
		case 0xC5:
			cycles = opcode_0xC5(memory, registers);
			break;
		case 0xC6:
			cycles = opcode_0xC6(memory, registers);
			break;
		case 0xC7:
			cycles = opcode_0xC7(memory, registers);
			break;
		case 0xC8:
			cycles = opcode_0xC8(memory, registers);
			break;
		case 0xC9:
			cycles = opcode_0xC9(memory, registers);
			break;
		case 0xCA:
			cycles = opcode_0xCA(memory, registers);
			break;
		case 0xCC:
			cycles = opcode_0xCC(memory, registers);
			break;
		case 0xCD:
			cycles = opcode_0xCD(memory, registers);
			break;
		case 0xCE:
			cycles = opcode_0xCE(memory, registers);
			break;
		case 0xCF:
			cycles = opcode_0xCF(memory, registers);
			break;
		case 0xD0:
			cycles = opcode_0xD0(memory, registers);
			break;
		case 0xD1:
			cycles = opcode_0xD1(memory, registers);
			break;
		case 0xD2:
			cycles = opcode_0xD2(memory, registers);
			break;
		case 0xD4:
			cycles = opcode_0xD4(memory, registers);
			break;
		case 0xD5:
			cycles = opcode_0xD5(memory, registers);
			break;
		case 0xD6:
			cycles = opcode_0xD6(memory, registers);
			break;
		case 0xD7:
			cycles = opcode_0xD7(memory, registers);
			break;
		case 0xD8:
			cycles = opcode_0xD8(memory, registers);
			break;
		case 0xD9:
			cycles = opcode_0xD9(memory, registers);
			break;
		case 0xDA:
			cycles = opcode_0xDA(memory, registers);
			break;
		case 0xDC:
			cycles = opcode_0xDC(memory, registers);
			break;
		case 0xDE:
			cycles = opcode_0xDE(memory, registers);
			break;
		case 0xDF:
			cycles = opcode_0xDF(memory, registers);
			break;
		case 0xE0:
			cycles = opcode_0xE0(memory, registers);
			break;
		case 0xE1:
			cycles = opcode_0xE1(memory, registers);
			break;
		case 0xE2:
			cycles = opcode_0xE2(memory, registers);
			break;
		case 0xE5:
			cycles = opcode_0xE5(memory, registers);
			break;
		case 0xE6:
			cycles = opcode_0xE6(memory, registers);
			break;
		case 0xE7:
			cycles = opcode_0xE7(memory, registers);
			break;
		case 0xE8:
			cycles = opcode_0xE8(memory, registers);
			break;
		case 0xE9:
			cycles = opcode_0xE9(memory, registers);
			break;
		case 0xEA:
			cycles = opcode_0xEA(memory, registers);
			break;
		case 0xEE:
			cycles = opcode_0xEE(memory, registers);
			break;
		case 0xEF:
			cycles = opcode_0xEF(memory, registers);
			break;
		case 0xF0:
			cycles = opcode_0xF0(memory, registers);
			break;
		case 0xF1:
			cycles = opcode_0xF1(memory, registers);
			break;
		case 0xF2:
			cycles = opcode_0xF2(memory, registers);
			break;
		case 0xF3:
			cycles = opcode_0xF3();
			break;
		case 0xF5:
			cycles = opcode_0xF5(memory, registers);
			break;
		case 0xF6:
			cycles = opcode_0xF6(memory, registers);
			break;
		case 0xF7:
			cycles = opcode_0xF7(memory, registers);
			break;
		case 0xF8:
			cycles = opcode_0xF8(memory, registers);
			break;
		case 0xF9:
			cycles = opcode_0xF9(registers);
			break;
		case 0xFA:
			cycles = opcode_0xFA(memory, registers);
			break;
		case 0xFB:
			cycles = opcode_0xFB();
			break;
		case 0xFE:
			cycles = opcode_0xFE(memory, registers);
			break;
		case 0xFF:
			cycles = opcode_0xFF(memory, registers);
			break;
		default:
			break;
	}

	return cycles;
}
