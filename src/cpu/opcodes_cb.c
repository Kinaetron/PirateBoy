#include "cpu/cpu.h"
#include "cpu/opcodes_cb.h"

static uint8_t rotate_bits_left(CPU_Memory* memory, uint8_t* value, uint8_t carry_bit, uint8_t flag_bit)
{
	uint8_t a = *value;

	a = (a << 1) | carry_bit;
	*value = a;

	set_register_flag(memory, Z, a == 0x00);
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, false);
	set_register_flag(memory, C, flag_bit);

	return 8;
}

static uint8_t rotate_bits_right(CPU_Memory* memory, uint8_t* value, uint8_t carry_bit, uint8_t flag_bit)
{
	uint8_t a = *value;

	a = (a >> 1) | (carry_bit << 7);
	*value = a;

	set_register_flag(memory, Z, a == 0x00);
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, false);
	set_register_flag(memory, C, flag_bit);

	return 8;
}

static uint8_t rotate_bits_left_opcode(CPU_Memory* memory, uint8_t* value)
{
	uint8_t carry_bit = (*value >> 7) & 0x01;

	return rotate_bits_left(memory, value, carry_bit, carry_bit);
}

static uint8_t rotate_bits_left_c_flag_opcode(CPU_Memory* memory, uint8_t* value)
{
	uint8_t carry_bit = (*value >> 7) & 0x01;
	uint8_t c_flag = get_register_flag(memory, C);

	return rotate_bits_left(memory, value, c_flag, carry_bit);
}

static uint8_t rotate_bits_right_opcode(CPU_Memory* memory, uint8_t* value)
{
	uint8_t carry_bit = *value & 0x01;

	return rotate_bits_right(memory, value, carry_bit, carry_bit);
}

static uint8_t rotate_bits_right_c_flag_opcode(CPU_Memory* memory, uint8_t* value)
{
	uint8_t carry_bit = *value & 0x01;
	uint8_t c_flag = get_register_flag(memory, C);

	return rotate_bits_right(memory, value, c_flag, carry_bit);
}

static uint8_t opcode_0x00(CPU_Memory* memory) {
	return rotate_bits_left_opcode(memory, &memory->bc.high);
}

static uint8_t opcode_0x01(CPU_Memory* memory) {
	return rotate_bits_left_opcode(memory, &memory->bc.low);
}

static uint8_t opcode_0x02(CPU_Memory* memory) {
	return rotate_bits_left_opcode(memory, &memory->de.high);
}

static uint8_t opcode_0x03(CPU_Memory* memory) {
	return rotate_bits_left_opcode(memory, &memory->de.low);
}

static uint8_t opcode_0x04(CPU_Memory* memory) {
	return rotate_bits_left_opcode(memory, &memory->hl.high);
}

static uint8_t opcode_0x05(CPU_Memory* memory) {
	return rotate_bits_left_opcode(memory, &memory->hl.low);
}

static uint8_t opcode_0x06(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	rotate_bits_left_opcode(memory, &value);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0x07(CPU_Memory* memory) {
	return rotate_bits_left_opcode(memory, &memory->af.high);
}

static uint8_t opcode_0x08(CPU_Memory* memory) {
	return rotate_bits_right_opcode(memory, &memory->bc.high);
}

static uint8_t opcode_0x09(CPU_Memory* memory) {
	return rotate_bits_right_opcode(memory, &memory->bc.low);
}

static uint8_t opcode_0x0A(CPU_Memory* memory) {
	return rotate_bits_right_opcode(memory, &memory->de.high);
}

static uint8_t opcode_0x0B(CPU_Memory* memory) {
	return rotate_bits_right_opcode(memory, &memory->de.low);
}

static uint8_t opcode_0x0C(CPU_Memory* memory) {
	return rotate_bits_right_opcode(memory, &memory->hl.high);
}

static uint8_t opcode_0x0D(CPU_Memory* memory) {
	return rotate_bits_right_opcode(memory, &memory->hl.low);
}

static uint8_t opcode_0x0E(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	rotate_bits_right_opcode(memory, &value);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0x0F(CPU_Memory* memory) {
	return rotate_bits_right_opcode(memory, &memory->af.high);
}

static uint8_t opcode_0x10(CPU_Memory* memory) {
	return rotate_bits_left_c_flag_opcode(memory, &memory->bc.high);
}

static uint8_t opcode_0x11(CPU_Memory* memory) {
	return rotate_bits_left_c_flag_opcode(memory, &memory->bc.low);
}

static uint8_t opcode_0x12(CPU_Memory* memory) {
	return rotate_bits_left_c_flag_opcode(memory, &memory->de.high);
}

static uint8_t opcode_0x13(CPU_Memory* memory) {
	return rotate_bits_left_c_flag_opcode(memory, &memory->de.low);
}

static uint8_t opcode_0x14(CPU_Memory* memory) {
	return rotate_bits_left_c_flag_opcode(memory, &memory->hl.high);
}

static uint8_t opcode_0x15(CPU_Memory* memory) {
	return rotate_bits_left_c_flag_opcode(memory, &memory->hl.low);
}

static uint8_t opcode_0x16(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	rotate_bits_left_c_flag_opcode(memory, &value);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0x17(CPU_Memory* memory) {
	return rotate_bits_left_c_flag_opcode(memory, &memory->af.high);
}

static uint8_t opcode_0x18(CPU_Memory* memory) {
	return rotate_bits_right_c_flag_opcode(memory, &memory->bc.high);
}

static uint8_t opcode_0x19(CPU_Memory* memory) {
	return rotate_bits_right_c_flag_opcode(memory, &memory->bc.low);
}

static uint8_t opcode_0x1A(CPU_Memory* memory) {
	return rotate_bits_right_c_flag_opcode(memory, &memory->de.high);
}

static uint8_t opcode_0x1B(CPU_Memory* memory) {
	return rotate_bits_right_c_flag_opcode(memory, &memory->de.low);
}

static uint8_t opcode_0x1C(CPU_Memory* memory) {
	return rotate_bits_right_c_flag_opcode(memory, &memory->hl.high);
}

static uint8_t opcode_0x1D(CPU_Memory* memory) {
	return rotate_bits_right_c_flag_opcode(memory, &memory->hl.low);
}

static uint8_t opcode_0x1E(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	rotate_bits_right_c_flag_opcode(memory, &value);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0x1F(CPU_Memory* memory) {
	return rotate_bits_right_c_flag_opcode(memory, &memory->af.high);
}

uint8_t opcode_cb_step(CPU_Memory* memory)
{
	uint8_t opcode = fetch_byte(memory, &memory->program_counter);

	uint8_t cycles = 4;

	switch (opcode)
	{
		case 0x00:
			cycles = opcode_0x00(memory);
			break;
		case 0x01:
			cycles = opcode_0x01(memory);
			break;
		case 0x02:
			cycles = opcode_0x02(memory);
			break;
		case 0x03:
			cycles = opcode_0x03(memory);
			break;
		case 0x04:
			cycles = opcode_0x04(memory);
			break;
		case 0x05:
			cycles = opcode_0x05(memory);
			break;
		case 0x06:
			cycles = opcode_0x06(memory);
			break;
		case 0x07:
			cycles = opcode_0x07(memory);
			break;
		case 0x08:
			cycles = opcode_0x08(memory);
			break;
		case 0x09:
			cycles = opcode_0x09(memory);
			break;
		case 0x0A:
			cycles = opcode_0x0A(memory);
			break;
		case 0x0B:
			cycles = opcode_0x0B(memory);
			break;
		case 0x0C:
			cycles = opcode_0x0C(memory);
			break;
		case 0x0D:
			cycles = opcode_0x0D(memory);
			break;
		case 0x0E:
			cycles = opcode_0x0E(memory);
			break;
		case 0x0F:
			cycles = opcode_0x0F(memory);
			break;
		case 0x10:
			cycles = opcode_0x10(memory);
			break;
		case 0x11:
			cycles = opcode_0x11(memory);
			break;
		case 0x12:
			cycles = opcode_0x12(memory);
			break;
		case 0x13:
			cycles = opcode_0x13(memory);
			break;
		case 0x14:
			cycles = opcode_0x14(memory);
			break;
		case 0x15:
			cycles = opcode_0x15(memory);
			break;
		case 0x16:
			cycles = opcode_0x16(memory);
			break;
		case 0x17:
			cycles = opcode_0x17(memory);
			break;
		case 0x18:
			cycles = opcode_0x18(memory);
			break;
		case 0x19:
			cycles = opcode_0x19(memory);
			break;
		case 0x1A:
			cycles = opcode_0x1A(memory);
			break;
		case 0x1B:
			cycles = opcode_0x1B(memory);
			break;
		case 0x1C:
			cycles = opcode_0x1C(memory);
			break;
		case 0x1D:
			cycles = opcode_0x1D(memory);
			break;
		case 0x1E:
			cycles = opcode_0x1E(memory);
			break;
		case 0x1F:
			cycles = opcode_0x1F(memory);
			break;
		default:
			break;
	}

	return cycles;
}