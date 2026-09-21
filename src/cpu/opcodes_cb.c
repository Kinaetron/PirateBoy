#include "cpu/cpu.h"
#include "cpu/opcodes_cb.h"

static uint8_t set_bit_to_zero(Memory* memory, uint8_t* value, uint8_t bit_to_set)
{
	uint8_t a = *value;

	a = a & ~(0x01 << bit_to_set);
	*value = a;

	return 8;
}

static uint8_t set_bit_to_one(Memory* memory, uint8_t* value, uint8_t bit_to_set)
{
	uint8_t a = *value;

	a = a | (0x01 << bit_to_set);
	*value = a;

	return 8;
}

static uint8_t rotate_bits_left(Memory* memory, Register* registers, uint8_t* value, uint8_t carry_bit, uint8_t flag_bit)
{
	uint8_t a = *value;

	a = (a << 1) | carry_bit;
	*value = a;

	set_register_flag(registers, Z, a == 0x00);
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, false);
	set_register_flag(registers, C, flag_bit);

	return 8;
}

static uint8_t rotate_bits_right(Memory* memory, Register* registers, uint8_t* value, uint8_t carry_bit, uint8_t flag_bit)
{
	uint8_t a = *value;

	a = (a >> 1) | (carry_bit << 7);
	*value = a;

	set_register_flag(registers, Z, a == 0x00);
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, false);
	set_register_flag(registers, C, flag_bit);

	return 8;
}

static uint8_t shift_bits_right(Memory* memory, Register* registers, uint8_t* value, uint8_t bit_7, uint8_t carry_bit)
{
	uint8_t a = *value;

	a = (a >> 1) | bit_7;
	*value = a;

	set_register_flag(registers, Z, a == 0x00);
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, false);
	set_register_flag(registers, C, carry_bit);

	return 8;
}

static uint8_t bit_check(Memory* memory,Register* registers, uint8_t value, uint8_t shift_by)
{
	uint8_t bit_0 = (value >> shift_by) & 0x01;

	set_register_flag(registers, Z, !bit_0);
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, true);

	return 8;
}

static uint8_t swap_nibbles_opcode(Memory* memory, Register* registers, uint8_t* value)
{
	uint8_t a = *value;
	uint8_t nibble_left = (a << 4) & 0xF0;
	uint8_t nibble_right = (a >> 4) & 0x0F;

	a = nibble_left | nibble_right;
	*value = a;

	set_register_flag(registers, Z, a == 0x00);
	set_register_flag(registers, N, false);
	set_register_flag(registers, H, false);
	set_register_flag(registers, C, false);

	return 8;
}

static uint8_t shift_bits_right_opcode(Memory* memory, Register* registers, uint8_t* value)
{
	uint8_t a = *value;
	uint8_t carry_bit = *value & 0x01;
	uint8_t bit_7 = a & 0x80;

	return shift_bits_right(memory, registers, value, bit_7, carry_bit);
}

static uint8_t shift_bits_right_logical_opcode(Memory* memory, Register* registers, uint8_t* value)
{
	uint8_t a = *value;
	uint8_t carry_bit = *value & 0x01;

	return shift_bits_right(memory, registers, value, 0x00, carry_bit);
}

static uint8_t rotate_bits_left_opcode(Memory* memory, Register* registers, uint8_t* value)
{
	uint8_t carry_bit = (*value >> 7) & 0x01;

	return rotate_bits_left(memory, registers, value, carry_bit, carry_bit);
}

static uint8_t rotate_bits_left_c_flag_opcode(Memory* memory, Register* registers, uint8_t* value)
{
	uint8_t carry_bit = (*value >> 7) & 0x01;
	uint8_t c_flag = get_register_flag(memory, C);

	return rotate_bits_left(memory,registers, value, c_flag, carry_bit);
}

static uint8_t rotate_bits_right_opcode(Memory* memory, Register* registers, uint8_t* value)
{
	uint8_t carry_bit = *value & 0x01;

	return rotate_bits_right(memory, registers, value, carry_bit, carry_bit);
}

static uint8_t rotate_bits_right_c_flag_opcode(Memory* memory, Register* registers, uint8_t* value)
{
	uint8_t carry_bit = *value & 0x01;
	uint8_t c_flag = get_register_flag(memory, C);

	return rotate_bits_right(memory,registers, value, c_flag, carry_bit);
}

static uint8_t rotate_bits_left_flag_zero_opcode(Memory* memory, Register* registers, uint8_t* value)
{
	uint8_t carry_bit = (*value >> 7) & 0x01;

	return rotate_bits_left(memory, registers, value, 0, carry_bit);
}

static uint8_t opcode_0x00(Memory* memory, Register* registers) {
	return rotate_bits_left_opcode(memory, registers, &registers->bc.high);
}

static uint8_t opcode_0x01(Memory* memory, Register* registers) {
	return rotate_bits_left_opcode(memory, registers, &registers->bc.low);
}

static uint8_t opcode_0x02(Memory* memory, Register* registers) {
	return rotate_bits_left_opcode(memory, registers, &registers->de.high);
}

static uint8_t opcode_0x03(Memory* memory, Register* registers) {
	return rotate_bits_left_opcode(memory, registers, &registers->de.low);
}

static uint8_t opcode_0x04(Memory* memory, Register* registers) {
	return rotate_bits_left_opcode(memory, registers, &registers->hl.high);
}

static uint8_t opcode_0x05(Memory* memory, Register* registers) {
	return rotate_bits_left_opcode(memory, registers, &registers->hl.low);
}

static uint8_t opcode_0x06(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	rotate_bits_left_opcode(memory, registers, &value);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0x07(Memory* memory, Register* registers) {
	return rotate_bits_left_opcode(memory, registers, &registers->af.high);
}

static uint8_t opcode_0x08(Memory* memory, Register* registers) {
	return rotate_bits_right_opcode(memory, registers, &registers->bc.high);
}

static uint8_t opcode_0x09(Memory* memory, Register* registers) {
	return rotate_bits_right_opcode(memory,registers, &registers->bc.low);
}

static uint8_t opcode_0x0A(Memory* memory, Register* registers) {
	return rotate_bits_right_opcode(memory,registers, &registers->de.high);
}

static uint8_t opcode_0x0B(Memory* memory, Register* registers) {
	return rotate_bits_right_opcode(memory, registers, &registers->de.low);
}

static uint8_t opcode_0x0C(Memory* memory, Register* registers) {
	return rotate_bits_right_opcode(memory,registers, &registers->hl.high);
}

static uint8_t opcode_0x0D(Memory* memory, Register* registers) {
	return rotate_bits_right_opcode(memory,registers, &registers->hl.low);
}

static uint8_t opcode_0x0E(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	rotate_bits_right_opcode(memory, registers, &value);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0x0F(Memory* memory, Register* registers) {
	return rotate_bits_right_opcode(memory, registers, &registers->af.high);
}

static uint8_t opcode_0x10(Memory* memory, Register* registers) {
	return rotate_bits_left_c_flag_opcode(memory, registers, &registers->bc.high);
}

static uint8_t opcode_0x11(Memory* memory, Register* registers) {
	return rotate_bits_left_c_flag_opcode(memory, registers, &registers->bc.low);
}

static uint8_t opcode_0x12(Memory* memory, Register* registers) {
	return rotate_bits_left_c_flag_opcode(memory, registers, &registers->de.high);
}

static uint8_t opcode_0x13(Memory* memory, Register* registers) {
	return rotate_bits_left_c_flag_opcode(memory, registers, &registers->de.low);
}

static uint8_t opcode_0x14(Memory* memory, Register* registers) {
	return rotate_bits_left_c_flag_opcode(memory,registers, &registers->hl.high);
}

static uint8_t opcode_0x15(Memory* memory, Register* registers) {
	return rotate_bits_left_c_flag_opcode(memory, registers, &registers->hl.low);
}

static uint8_t opcode_0x16(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	rotate_bits_left_c_flag_opcode(memory, registers, &value);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0x17(Memory* memory, Register* registers) {
	return rotate_bits_left_c_flag_opcode(memory, registers, &registers->af.high);
}

static uint8_t opcode_0x18(Memory* memory, Register* registers) {
	return rotate_bits_right_c_flag_opcode(memory, registers, &registers->bc.high);
}

static uint8_t opcode_0x19(Memory* memory, Register* registers) {
	return rotate_bits_right_c_flag_opcode(memory, registers, &registers->bc.low);
}

static uint8_t opcode_0x1A(Memory* memory, Register* registers) {
	return rotate_bits_right_c_flag_opcode(memory, registers, &registers->de.high);
}

static uint8_t opcode_0x1B(Memory* memory, Register* registers) {
	return rotate_bits_right_c_flag_opcode(memory, registers, &registers->de.low);
}

static uint8_t opcode_0x1C(Memory* memory, Register* registers) {
	return rotate_bits_right_c_flag_opcode(memory, registers, &registers->hl.high);
}

static uint8_t opcode_0x1D(Memory* memory, Register* registers) {
	return rotate_bits_right_c_flag_opcode(memory, registers, &registers->hl.low);
}

static uint8_t opcode_0x1E(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	rotate_bits_right_c_flag_opcode(memory, registers, &value);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0x1F(Memory* memory, Register* registers) {
	return rotate_bits_right_c_flag_opcode(memory, registers, &registers->af.high);
}

static uint8_t opcode_0x20(Memory* memory, Register* registers) {
	return rotate_bits_left_flag_zero_opcode(memory, registers, &registers->bc.high);
}

static uint8_t opcode_0x21(Memory* memory, Register* registers) {
	return rotate_bits_left_flag_zero_opcode(memory, registers, &registers->bc.low);
}

static uint8_t opcode_0x22(Memory* memory, Register* registers) {
	return rotate_bits_left_flag_zero_opcode(memory, registers, &registers->de.high);
}

static uint8_t opcode_0x23(Memory* memory, Register* registers) {
	return rotate_bits_left_flag_zero_opcode(memory, registers, &registers->de.low);
}

static uint8_t opcode_0x24(Memory* memory, Register* registers) {
	return rotate_bits_left_flag_zero_opcode(memory, registers, &registers->hl.high);
}

static uint8_t opcode_0x25(Memory* memory, Register* registers) {
	return rotate_bits_left_flag_zero_opcode(memory, registers, &registers->hl.low);
}

static uint8_t opcode_0x26(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	rotate_bits_left_flag_zero_opcode(memory, registers, &value);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0x27(Memory* memory, Register* registers) {
	return rotate_bits_left_flag_zero_opcode(memory, registers, &registers->af.high);
}

static uint8_t opcode_0x28(Memory* memory, Register* registers) {
	return shift_bits_right_opcode(memory, registers, &registers->bc.high);
}

static uint8_t opcode_0x29(Memory* memory, Register* registers) {
	return shift_bits_right_opcode(memory, registers, &registers->bc.low);
}

static uint8_t opcode_0x2A(Memory* memory, Register* registers) {
	return shift_bits_right_opcode(memory, registers, &registers->de.high);
}

static uint8_t opcode_0x2B(Memory* memory, Register* registers) {
	return shift_bits_right_opcode(memory, registers, &registers->de.low);
}

static uint8_t opcode_0x2C(Memory* memory, Register* registers) {
	return shift_bits_right_opcode(memory, registers, &registers->hl.high);
}

static uint8_t opcode_0x2D(Memory* memory, Register* registers) {
	return shift_bits_right_opcode(memory, registers, &registers->hl.low);
}

static uint8_t opcode_0x2E(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	shift_bits_right_opcode(memory, registers, &value);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0x2F(Memory* memory, Register* registers) {
	return shift_bits_right_opcode(memory, registers, &registers->af.high);
}

static uint8_t opcode_0x30(Memory* memory, Register* registers) {
	return swap_nibbles_opcode(memory, registers, &registers->bc.high);
}

static uint8_t opcode_0x31(Memory* memory, Register* registers) {
	return swap_nibbles_opcode(memory, registers, &registers->bc.low);
}

static uint8_t opcode_0x32(Memory* memory, Register* registers) {
	return swap_nibbles_opcode(memory, registers, &registers->de.high);
}

static uint8_t opcode_0x33(Memory* memory, Register* registers) {
	return swap_nibbles_opcode(memory, registers, &registers->de.low);
}

static uint8_t opcode_0x34(Memory* memory, Register* registers) {
	return swap_nibbles_opcode(memory, registers, &registers->hl.high);
}

static uint8_t opcode_0x35(Memory* memory, Register* registers) {
	return swap_nibbles_opcode(memory, registers, &registers->hl.low);
}

static uint8_t opcode_0x36(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	swap_nibbles_opcode(memory, registers, &value);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0x37(Memory* memory, Register* registers) {
	return swap_nibbles_opcode(memory, registers, &registers->af.high);
}

static uint8_t opcode_0x38(Memory* memory, Register* registers) {
	return shift_bits_right_logical_opcode(memory,  registers, &registers->bc.high);
}

static uint8_t opcode_0x39(Memory* memory, Register* registers) {
	return shift_bits_right_logical_opcode(memory,  registers, &registers->bc.low);
}

static uint8_t opcode_0x3A(Memory* memory, Register* registers) {
	return shift_bits_right_logical_opcode(memory, registers, &registers->de.high);
}

static uint8_t opcode_0x3B(Memory* memory, Register* registers) {
	return shift_bits_right_logical_opcode(memory, registers, &registers->de.low);
}

static uint8_t opcode_0x3C(Memory* memory, Register* registers) {
	return shift_bits_right_logical_opcode(memory, registers, &registers->hl.high);
}

static uint8_t opcode_0x3D(Memory* memory, Register* registers) {
	return shift_bits_right_logical_opcode(memory, registers, &registers->hl.low);
}

static uint8_t opcode_0x3E(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	shift_bits_right_logical_opcode(memory, registers, &value);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0x3F(Memory* memory, Register* registers) {
	return shift_bits_right_logical_opcode(memory, registers, &registers->af.high);
}

static uint8_t opcode_0x40(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.high, 0);
}

static uint8_t opcode_0x41(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.low, 0);
}

static uint8_t opcode_0x42(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.high, 0);
}

static uint8_t opcode_0x43(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.low, 0);
}

static uint8_t opcode_0x44(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.high, 0);
}

static uint8_t opcode_0x45(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.low, 0);
}

static uint8_t opcode_0x46(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	bit_check(memory, registers, value, 0);

	return 12;
}

static uint8_t opcode_0x47(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->af.high, 0);
}

static uint8_t opcode_0x48(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.high, 1);
}

static uint8_t opcode_0x49(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.low, 1);
}

static uint8_t opcode_0x4A(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.high, 1);
}

static uint8_t opcode_0x4B(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.low, 1);
}

static uint8_t opcode_0x4C(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.high, 1);
}

static uint8_t opcode_0x4D(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.low, 1);
}

static uint8_t opcode_0x4E(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	bit_check(memory, registers, value, 1);

	return 12;
}

static uint8_t opcode_0x4F(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->af.high, 1);
}

static uint8_t opcode_0x50(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.high, 2);
}

static uint8_t opcode_0x51(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.low, 2);
}

static uint8_t opcode_0x52(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.high, 2);
}

static uint8_t opcode_0x53(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.low, 2);
}

static uint8_t opcode_0x54(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.high, 2);
}

static uint8_t opcode_0x55(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.low, 2);
}

static uint8_t opcode_0x56(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	bit_check(memory, registers, value, 2);

	return 12;
}

static uint8_t opcode_0x57(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->af.high, 2);
}

static uint8_t opcode_0x58(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.high, 3);
}

static uint8_t opcode_0x59(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.low, 3);
}

static uint8_t opcode_0x5A(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.high, 3);
}

static uint8_t opcode_0x5B(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.low, 3);
}

static uint8_t opcode_0x5C(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.high, 3);
}

static uint8_t opcode_0x5D(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.low, 3);
}

static uint8_t opcode_0x5E(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	bit_check(memory, registers, value, 3);

	return 12;
}

static uint8_t opcode_0x5F(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->af.high, 3);
}

static uint8_t opcode_0x60(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.high, 4);
}

static uint8_t opcode_0x61(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.low, 4);
}

static uint8_t opcode_0x62(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.high, 4);
}

static uint8_t opcode_0x63(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.low, 4);
}

static uint8_t opcode_0x64(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.high, 4);
}

static uint8_t opcode_0x65(Memory* memory, Register* registers)  {
	return bit_check(memory, registers, registers->hl.low, 4);
}

static uint8_t opcode_0x66(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	bit_check(memory, registers, value, 4);

	return 12;
}

static uint8_t opcode_0x67(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->af.high, 4);
}

static uint8_t opcode_0x68(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.high, 5);
}

static uint8_t opcode_0x69(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.low, 5);
}

static uint8_t opcode_0x6A(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.high, 5);
}

static uint8_t opcode_0x6B(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.low , 5);
}

static uint8_t opcode_0x6C(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.high, 5);
}

static uint8_t opcode_0x6D(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.low, 5);
}

static uint8_t opcode_0x6E(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	bit_check(memory, registers, value, 5);

	return 12;
}

static uint8_t opcode_0x6F(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->af.high, 5);
}

static uint8_t opcode_0x70(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.high, 6);
}

static uint8_t opcode_0x71(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.low, 6);
}

static uint8_t opcode_0x72(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.high, 6);
}

static uint8_t opcode_0x73(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.low, 6);
}

static uint8_t opcode_0x74(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.high, 6);
}

static uint8_t opcode_0x75(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.low, 6);
}

static uint8_t opcode_0x76(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	bit_check(memory,registers, value, 6);

	return 12;
}

static uint8_t opcode_0x77(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->af.high, 6);
}

static uint8_t opcode_0x78(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.high, 7);
}

static uint8_t opcode_0x79(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->bc.low, 7);
}

static uint8_t opcode_0x7A(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.high, 7);
}

static uint8_t opcode_0x7B(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->de.low, 7);
}

static uint8_t opcode_0x7C(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.high, 7);
}

static uint8_t opcode_0x7D(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->hl.low, 7);
}

static uint8_t opcode_0x7E(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	bit_check(memory, registers, value, 7);

	return 12;
}

static uint8_t opcode_0x7F(Memory* memory, Register* registers) {
	return bit_check(memory, registers, registers->af.high, 7);
}

static uint8_t opcode_0x80(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.high, 0);
}

static uint8_t opcode_0x81(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.low, 0);
}

static uint8_t opcode_0x82(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.high, 0);
}

static uint8_t opcode_0x83(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.low, 0);
}

static uint8_t opcode_0x84(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.high, 0);
}

static uint8_t opcode_0x85(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.low, 0);
}

static uint8_t opcode_0x86(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_zero(memory, &value, 0);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0x87(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->af.high, 0);
}

static uint8_t opcode_0x88(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.high, 1);
}

static uint8_t opcode_0x89(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.low, 1);
}

static uint8_t opcode_0x8A(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.high, 1);
}

static uint8_t opcode_0x8B(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.low, 1);
}

static uint8_t opcode_0x8C(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.high, 1);
}

static uint8_t opcode_0x8D(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.low, 1);
}

static uint8_t opcode_0x8E(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_zero(memory, &value, 1);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0x8F(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->af.high, 1);
}

static uint8_t opcode_0x90(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.high, 2);
}

static uint8_t opcode_0x91(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.low, 2);
}

static uint8_t opcode_0x92(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.high, 2);
}

static uint8_t opcode_0x93(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.low, 2);
}

static uint8_t opcode_0x94(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.high, 2);
}

static uint8_t opcode_0x95(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.low, 2);
}

static uint8_t opcode_0x96(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_zero(memory, &value, 2);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0x97(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->af.high, 2);
}

static uint8_t opcode_0x98(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.high, 3);
}

static uint8_t opcode_0x99(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.low, 3);
}

static uint8_t opcode_0x9A(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.high, 3);
}

static uint8_t opcode_0x9B(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.low, 3);
}

static uint8_t opcode_0x9C(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.high, 3);
}

static uint8_t opcode_0x9D(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.low, 3);
}

static uint8_t opcode_0x9E(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_zero(memory, &value, 3);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0x9F(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->af.high, 3);
}

static uint8_t opcode_0xA0(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.high, 4);
}

static uint8_t opcode_0xA1(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.low, 4);
}

static uint8_t opcode_0xA2(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.high, 4);
}

static uint8_t opcode_0xA3(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.low, 4);
}

static uint8_t opcode_0xA4(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.high, 4);
}

static uint8_t opcode_0xA5(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.low, 4);
}

static uint8_t opcode_0xA6(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_zero(memory, &value, 4);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0xA7(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->af.high, 4);
}

static uint8_t opcode_0xA8(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.high, 5);
}

static uint8_t opcode_0xA9(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.low, 5);
}

static uint8_t opcode_0xAA(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.high, 5);
}

static uint8_t opcode_0xAB(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.low, 5);
}

static uint8_t opcode_0xAC(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.high, 5);
}

static uint8_t opcode_0xAD(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.low, 5);
}

static uint8_t opcode_0xAE(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_zero(memory, &value, 5);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0xAF(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->af.high, 5);
}

static uint8_t opcode_0xB0(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.high, 6);
}

static uint8_t opcode_0xB1(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.low, 6);
}

static uint8_t opcode_0xB2(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.high, 6);
}

static uint8_t opcode_0xB3(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.low, 6);
}

static uint8_t opcode_0xB4(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.high, 6);
}

static uint8_t opcode_0xB5(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.low, 6);
}

static uint8_t opcode_0xB6(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_zero(memory, &value, 6);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0xB7(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->af.high, 6);
}

static uint8_t opcode_0xB8(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.high, 7);
}

static uint8_t opcode_0xB9(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->bc.low, 7);
}

static uint8_t opcode_0xBA(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.high, 7);
}

static uint8_t opcode_0xBB(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->de.low, 7);
}

static uint8_t opcode_0xBC(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.high, 7);
}

static uint8_t opcode_0xBD(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->hl.low, 7);
}

static uint8_t opcode_0xBE(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_zero(memory, &value, 7);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0xBF(Memory* memory, Register* registers) {
	return set_bit_to_zero(memory, &registers->af.high, 7);
}

static uint8_t opcode_0xC0(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.high, 0);
}

static uint8_t opcode_0xC1(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.low, 0);
}

static uint8_t opcode_0xC2(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.high, 0);
}

static uint8_t opcode_0xC3(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.low, 0);
}

static uint8_t opcode_0xC4(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.high, 0);
}

static uint8_t opcode_0xC5(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.low, 0);
}

static uint8_t opcode_0xC6(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_one(memory, &value, 0);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0xC7(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->af.high, 0);
}

static uint8_t opcode_0xC8(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.high, 1);
}

static uint8_t opcode_0xC9(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.low, 1);
}

static uint8_t opcode_0xCA(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.high, 1);
}

static uint8_t opcode_0xCB(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.low, 1);
}

static uint8_t opcode_0xCC(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.high, 1);
}

static uint8_t opcode_0xCD(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.low, 1);
}

static uint8_t opcode_0xCE(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_one(memory, &value, 1);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0xCF(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->af.high, 1);
}

static uint8_t opcode_0xD0(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.high, 2);
}

static uint8_t opcode_0xD1(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.low, 2);
}

static uint8_t opcode_0xD2(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.high, 2);
}

static uint8_t opcode_0xD3(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.low, 2);
}

static uint8_t opcode_0xD4(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.high, 2);
}

static uint8_t opcode_0xD5(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.low, 2);
}

static uint8_t opcode_0xD6(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_one(memory, &value, 2);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0xD7(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->af.high, 2);
}

static uint8_t opcode_0xD8(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.high, 3);
}

static uint8_t opcode_0xD9(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.low, 3);
}

static uint8_t opcode_0xDA(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.high, 3);
}

static uint8_t opcode_0xDB(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.low, 3);
}

static uint8_t opcode_0xDC(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.high, 3);
}

static uint8_t opcode_0xDD(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.low, 3);
}

static uint8_t opcode_0xDE(Memory* memory, Register* registers) 
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_one(memory, &value, 3);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0xDF(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->af.high, 3);
}

static uint8_t opcode_0xE0(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.high, 4);
}

static uint8_t opcode_0xE1(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.low, 4);
}

static uint8_t opcode_0xE2(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.high, 4);
}

static uint8_t opcode_0xE3(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.low, 4);
}

static uint8_t opcode_0xE4(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.high, 4);
}

static uint8_t opcode_0xE5(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.low, 4);
}

static uint8_t opcode_0xE6(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_one(memory, &value, 4);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0xE7(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->af.high, 4);
}

static uint8_t opcode_0xE8(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.high,5);
}

static uint8_t opcode_0xE9(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.low, 5);
}

static uint8_t opcode_0xEA(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.high, 5);
}

static uint8_t opcode_0xEB(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.low, 5);
}

static uint8_t opcode_0xEC(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.high, 5);
}

static uint8_t opcode_0xED(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.low, 5);
}

static uint8_t opcode_0xEE(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_one(memory, &value, 5);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0xEF(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->af.high, 5);
}

static uint8_t opcode_0xF0(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.high, 6);
}

static uint8_t opcode_0xF1(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.low, 6);
}

static uint8_t opcode_0xF2(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.high, 6);
}

static uint8_t opcode_0xF3(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.low, 6);
}

static uint8_t opcode_0xF4(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.high, 6);
}

static uint8_t opcode_0xF5(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.low, 6);
}

static uint8_t opcode_0xF6(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_one(memory, &value, 6);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0xF7(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->af.high, 6);
}

static uint8_t opcode_0xF8(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.high, 7);
}

static uint8_t opcode_0xF9(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->bc.low, 7);
}

static uint8_t opcode_0xFA(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.high, 7);
}

static uint8_t opcode_0xFB(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->de.low, 7);
}

static uint8_t opcode_0xFC(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.high, 7);
}

static uint8_t opcode_0xFD(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->hl.low, 7);
}

static uint8_t opcode_0xFE(Memory* memory, Register* registers)
{
	uint8_t value = memory_read(memory, registers->hl.value);

	set_bit_to_one(memory, &value, 7);
	memory_write(memory, registers->hl.value, value);

	return 16;
}

static uint8_t opcode_0xFF(Memory* memory, Register* registers) {
	return set_bit_to_one(memory, &registers->af.high, 7);
}

uint8_t opcode_cb_step(Memory* memory, Register* registers)
{
	uint8_t opcode = fetch_byte(memory, &registers->program_counter);

	uint8_t cycles = 4;

	switch (opcode)
	{
		case 0x00:
			cycles = opcode_0x00(memory, registers);
			break;
		case 0x01:
			cycles = opcode_0x01(memory, registers);
			break;
		case 0x02:
			cycles = opcode_0x02(memory, registers);
			break;
		case 0x03:
			cycles = opcode_0x03(memory, registers);
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
			cycles = opcode_0x09(memory, registers);
			break;
		case 0x0A:
			cycles = opcode_0x0A(memory, registers);
			break;
		case 0x0B:
			cycles = opcode_0x0B(memory, registers);
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
			cycles = opcode_0x0F(memory, registers);
			break;
		case 0x10:
			cycles = opcode_0x10(memory, registers);
			break;
		case 0x11:
			cycles = opcode_0x11(memory, registers);
			break;
		case 0x12:
			cycles = opcode_0x12(memory, registers);
			break;
		case 0x13:
			cycles = opcode_0x13(memory, registers);
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
			cycles = opcode_0x2B(memory, registers);
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
			cycles = opcode_0x33(memory, registers);
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
			cycles = opcode_0x37(memory, registers);
			break;
		case 0x38:
			cycles = opcode_0x38(memory, registers);
			break;
		case 0x39:
			cycles = opcode_0x39(memory, registers);
			break;
		case 0x3A:
			cycles = opcode_0x3A(memory, registers);
			break;
		case 0x3B:
			cycles = opcode_0x3B(memory, registers);
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
			cycles = opcode_0x3F(memory, registers);
			break;
		case 0x40:
			cycles = opcode_0x40(memory, registers);
			break;
		case 0x41:
			cycles = opcode_0x41(memory, registers);
			break;
		case 0x42:
			cycles = opcode_0x42(memory, registers);
			break;
		case 0x43:
			cycles = opcode_0x43(memory, registers);
			break;
		case 0x44:
			cycles = opcode_0x44(memory, registers);
			break;
		case 0x45:
			cycles = opcode_0x45(memory, registers);
			break;
		case 0x46:
			cycles = opcode_0x46(memory, registers);
			break;
		case 0x47:
			cycles = opcode_0x47(memory, registers);
			break;
		case 0x48:
			cycles = opcode_0x48(memory, registers);
			break;
		case 0x49:
			cycles = opcode_0x49(memory, registers);
			break;
		case 0x4A:
			cycles = opcode_0x4A(memory, registers);
			break;
		case 0x4B:
			cycles = opcode_0x4B(memory, registers);
			break;
		case 0x4C:
			cycles = opcode_0x4C(memory, registers);
			break;
		case 0x4D:
			cycles = opcode_0x4D(memory, registers);
			break;
		case 0x4E:
			cycles = opcode_0x4E(memory, registers);
			break;
		case 0x4F:
			cycles = opcode_0x4F(memory, registers);
			break;
		case 0x50:
			cycles = opcode_0x50(memory, registers);
			break;
		case 0x51:
			cycles = opcode_0x51(memory, registers);
			break;
		case 0x52:
			cycles = opcode_0x52(memory, registers);
			break;
		case 0x53:
			cycles = opcode_0x53(memory, registers);
			break;
		case 0x54:
			cycles = opcode_0x54(memory, registers);
			break;
		case 0x55:
			cycles = opcode_0x55(memory, registers);
			break;
		case 0x56:
			cycles = opcode_0x56(memory, registers);
			break;
		case 0x57:
			cycles = opcode_0x57(memory, registers);
			break;
		case 0x58:
			cycles = opcode_0x58(memory, registers);
			break;
		case 0x59:
			cycles = opcode_0x59(memory, registers);
			break;
		case 0x5A:
			cycles = opcode_0x5A(memory, registers);
			break;
		case 0x5B:
			cycles = opcode_0x5B(memory, registers);
			break;
		case 0x5C:
			cycles = opcode_0x5C(memory, registers);
			break;
		case 0x5D:
			cycles = opcode_0x5D(memory, registers);
			break;
		case 0x5E:
			cycles = opcode_0x5E(memory, registers);
			break;
		case 0x5F:
			cycles = opcode_0x5F(memory, registers);
			break;
		case 0x60:
			cycles = opcode_0x60(memory, registers);
			break;
		case 0x61:
			cycles = opcode_0x61(memory, registers);
			break;
		case 0x62:
			cycles = opcode_0x62(memory, registers);
			break;
		case 0x63:
			cycles = opcode_0x63(memory, registers);
			break;
		case 0x64:
			cycles = opcode_0x64(memory, registers);
			break;
		case 0x65:
			cycles = opcode_0x65(memory, registers);
			break;
		case 0x66:
			cycles = opcode_0x66(memory, registers);
			break;
		case 0x67:
			cycles = opcode_0x67(memory, registers);
			break;
		case 0x68:
			cycles = opcode_0x68(memory, registers);
			break;
		case 0x69:
			cycles = opcode_0x69(memory, registers);
			break;
		case 0x6A:
			cycles = opcode_0x6A(memory, registers);
			break;
		case 0x6B:
			cycles = opcode_0x6B(memory, registers);
			break;
		case 0x6C:
			cycles = opcode_0x6C(memory, registers);
			break;
		case 0x6D:
			cycles = opcode_0x6D(memory, registers);
			break;
		case 0x6E:
			cycles = opcode_0x6E(memory, registers);
			break;
		case 0x6F:
			cycles = opcode_0x6F(memory, registers);
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
			cycles = opcode_0x76(memory, registers);
			break;
		case 0x77:
			cycles = opcode_0x77(memory, registers);
			break;
		case 0x78:
			cycles = opcode_0x78(memory, registers);
			break;
		case 0x79:
			cycles = opcode_0x79(memory, registers);
			break;
		case 0x7A:
			cycles = opcode_0x7A(memory, registers);
			break;
		case 0x7B:
			cycles = opcode_0x7B(memory, registers);
			break;
		case 0x7C:
			cycles = opcode_0x7C(memory, registers);
			break;
		case 0x7D:
			cycles = opcode_0x7D(memory, registers);
			break;
		case 0x7E:
			cycles = opcode_0x7E(memory, registers);
			break;
		case 0x7F:
			cycles = opcode_0x7F(memory, registers);
			break;
		case 0x80:
			cycles = opcode_0x80(memory, registers);
			break;
		case 0x81:
			cycles = opcode_0x81(memory, registers);
			break;
		case 0x82:
			cycles = opcode_0x82(memory, registers);
			break;
		case 0x83:
			cycles = opcode_0x83(memory, registers);
			break;
		case 0x84:
			cycles = opcode_0x84(memory, registers);
			break;
		case 0x85:
			cycles = opcode_0x85(memory, registers);
			break;
		case 0x86:
			cycles = opcode_0x86(memory, registers);
			break;
		case 0x87:
			cycles = opcode_0x87(memory, registers);
			break;
		case 0x88:
			cycles = opcode_0x88(memory, registers);
			break;
		case 0x89:
			cycles = opcode_0x89(memory, registers);
			break;
		case 0x8A:
			cycles = opcode_0x8A(memory, registers);
			break;
		case 0x8B:
			cycles = opcode_0x8B(memory, registers);
			break;
		case 0x8C:
			cycles = opcode_0x8C(memory, registers);
			break;
		case 0x8D:
			cycles = opcode_0x8D(memory, registers);
			break;
		case 0x8E:
			cycles = opcode_0x8E(memory, registers);
			break;
		case 0x8F:
			cycles = opcode_0x8F(memory, registers);
			break;
		case 0x90:
			cycles = opcode_0x90(memory, registers);
			break;
		case 0x91:
			cycles = opcode_0x91(memory, registers);
			break;
		case 0x92:
			cycles = opcode_0x92(memory, registers);
			break;
		case 0x93:
			cycles = opcode_0x93(memory, registers);
			break;
		case 0x94:
			cycles = opcode_0x94(memory, registers);
			break;
		case 0x95:
			cycles = opcode_0x95(memory, registers);
			break;
		case 0x96:
			cycles = opcode_0x96(memory, registers);
			break;
		case 0x97:
			cycles = opcode_0x97(memory, registers);
			break;
		case 0x98:
			cycles = opcode_0x98(memory, registers);
			break;
		case 0x99:
			cycles = opcode_0x99(memory, registers);
			break;
		case 0x9A:
			cycles = opcode_0x9A(memory, registers);
			break;
		case 0x9B:
			cycles = opcode_0x9B(memory, registers);
			break;
		case 0x9C:
			cycles = opcode_0x9C(memory, registers);
			break;
		case 0x9D:
			cycles = opcode_0x9D(memory, registers);
			break;
		case 0x9E:
			cycles = opcode_0x9E(memory, registers);
			break;
		case 0x9F:
			cycles = opcode_0x9F(memory, registers);
			break;
		case 0xA0:
			cycles = opcode_0xA0(memory, registers);
			break;
		case 0xA1:
			cycles = opcode_0xA1(memory, registers);
			break;
		case 0xA2:
			cycles = opcode_0xA2(memory, registers);
			break;
		case 0xA3:
			cycles = opcode_0xA3(memory, registers);
			break;
		case 0xA4:
			cycles = opcode_0xA4(memory, registers);
			break;
		case 0xA5:
			cycles = opcode_0xA5(memory, registers);
			break;
		case 0xA6:
			cycles = opcode_0xA6(memory, registers);
			break;
		case 0xA7:
			cycles = opcode_0xA7(memory, registers);
			break;
		case 0xA8:
			cycles = opcode_0xA8(memory, registers);
			break;
		case 0xA9:
			cycles = opcode_0xA9(memory, registers);
			break;
		case 0xAA:
			cycles = opcode_0xAA(memory, registers);
			break;
		case 0xAB:
			cycles = opcode_0xAB(memory, registers);
			break;
		case 0xAC:
			cycles = opcode_0xAC(memory, registers);
			break;
		case 0xAD:
			cycles = opcode_0xAD(memory, registers);
			break;
		case 0xAE:
			cycles = opcode_0xAE(memory, registers);
			break;
		case 0xAF:
			cycles = opcode_0xAF(memory, registers);
			break;
		case 0xB0:
			cycles = opcode_0xB0(memory, registers);
			break;
		case 0xB1:
			cycles = opcode_0xB1(memory, registers);
			break;
		case 0xB2:
			cycles = opcode_0xB2(memory, registers);
			break;
		case 0xB3:
			cycles = opcode_0xB3(memory, registers);
			break;
		case 0xB4:
			cycles = opcode_0xB4(memory, registers);
			break;
		case 0xB5:
			cycles = opcode_0xB5(memory, registers);
			break;
		case 0xB6:
			cycles = opcode_0xB6(memory, registers);
			break;
		case 0xB7:
			cycles = opcode_0xB7(memory, registers);
			break;
		case 0xB8:
			cycles = opcode_0xB8(memory, registers);
			break;
		case 0xB9:
			cycles = opcode_0xB9(memory, registers);
			break;
		case 0xBA:
			cycles = opcode_0xBA(memory, registers);
			break;
		case 0xBB:
			cycles = opcode_0xBB(memory, registers);
			break;
		case 0xBC:
			cycles = opcode_0xBC(memory, registers);
			break;
		case 0xBD:
			cycles = opcode_0xBD(memory, registers);
			break;
		case 0xBE:
			cycles = opcode_0xBE(memory, registers);
			break;
		case 0xBF:
			cycles = opcode_0xBF(memory, registers);
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
		case 0xCB:
			cycles = opcode_0xCB(memory, registers);
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
		case 0xD3:
			cycles = opcode_0xD3(memory, registers);
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
		case 0xDB:
			cycles = opcode_0xDB(memory, registers);
			break;
		case 0xDC:
			cycles = opcode_0xDC(memory, registers);
			break;
		case 0xDD:
			cycles = opcode_0xDD(memory, registers);
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
		case 0xE3:
			cycles = opcode_0xE3(memory, registers);
			break;
		case 0xE4:
			cycles = opcode_0xE4(memory, registers);
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
		case 0xEB:
			cycles = opcode_0xEB(memory, registers);
			break;
		case 0xEC:
			cycles = opcode_0xEC(memory, registers);
			break;
		case 0xED:
			cycles = opcode_0xED(memory, registers);
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
			cycles = opcode_0xF3(memory, registers);
			break;
		case 0xF4:
			cycles = opcode_0xF4(memory, registers);
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
			cycles = opcode_0xF9(memory, registers);
			break;
		case 0xFA:
			cycles = opcode_0xFA(memory, registers);
			break;
		case 0xFB:
			cycles = opcode_0xFB(memory, registers);
			break;
		case 0xFC:
			cycles = opcode_0xFC(memory, registers);
			break;
		case 0xFD:
			cycles = opcode_0xFD(memory, registers);
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