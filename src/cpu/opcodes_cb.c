#include "cpu/cpu.h"
#include "cpu/opcodes_cb.h"

static uint8_t set_bit_to_zero(CPU_Memory* memory, uint8_t* value, uint8_t bit_to_set)
{
	uint8_t a = *value;

	a = a & ~(0x01 << bit_to_set);
	*value = a;

	return 8;
}

static uint8_t set_bit_to_one(CPU_Memory* memory, uint8_t* value, uint8_t bit_to_set)
{
	uint8_t a = *value;

	a = a | (0x01 << bit_to_set);
	*value = a;

	return 8;
}

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

static uint8_t shift_bits_right(CPU_Memory* memory, uint8_t* value, uint8_t bit_7, uint8_t carry_bit)
{
	uint8_t a = *value;

	a = (a >> 1) | bit_7;
	*value = a;

	set_register_flag(memory, Z, a == 0x00);
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, false);
	set_register_flag(memory, C, carry_bit);

	return 8;
}

static uint8_t bit_check(CPU_Memory* memory, uint8_t value, uint8_t shift_by)
{
	uint8_t bit_0 = (value >> shift_by) & 0x01;

	set_register_flag(memory, Z, !bit_0);
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, true);

	return 8;
}

static uint8_t swap_nibbles_opcode(CPU_Memory* memory, uint8_t* value)
{
	uint8_t a = *value;
	uint8_t nibble_left = (a << 4) & 0xF0;
	uint8_t nibble_right = (a >> 4) & 0x0F;

	a = nibble_left | nibble_right;
	*value = a;

	set_register_flag(memory, Z, a == 0x00);
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, false);
	set_register_flag(memory, C, false);

	return 8;
}

static uint8_t shift_bits_right_opcode(CPU_Memory* memory, uint8_t* value)
{
	uint8_t a = *value;
	uint8_t carry_bit = *value & 0x01;
	uint8_t bit_7 = a & 0x80;

	return shift_bits_right(memory, value, bit_7, carry_bit);
}

static uint8_t shift_bits_right_logical_opcode(CPU_Memory* memory, uint8_t* value)
{
	uint8_t a = *value;
	uint8_t carry_bit = *value & 0x01;

	return shift_bits_right(memory, value, 0x00, carry_bit);
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

static uint8_t rotate_bits_left_flag_zero_opcode(CPU_Memory* memory, uint8_t* value) 
{
	uint8_t carry_bit = (*value >> 7) & 0x01;

	return rotate_bits_left(memory, value, 0, carry_bit);
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

static uint8_t opcode_0x20(CPU_Memory* memory) {
	return rotate_bits_left_flag_zero_opcode(memory, &memory->bc.high);
}

static uint8_t opcode_0x21(CPU_Memory* memory) {
	return rotate_bits_left_flag_zero_opcode(memory, &memory->bc.low);
}

static uint8_t opcode_0x22(CPU_Memory* memory) {
	return rotate_bits_left_flag_zero_opcode(memory, &memory->de.high);
}

static uint8_t opcode_0x23(CPU_Memory* memory) {
	return rotate_bits_left_flag_zero_opcode(memory, &memory->de.low);
}

static uint8_t opcode_0x24(CPU_Memory* memory) {
	return rotate_bits_left_flag_zero_opcode(memory, &memory->hl.high);
}

static uint8_t opcode_0x25(CPU_Memory* memory) {
	return rotate_bits_left_flag_zero_opcode(memory, &memory->hl.low);
}

static uint8_t opcode_0x26(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	rotate_bits_left_flag_zero_opcode(memory, &value);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0x27(CPU_Memory* memory) {
	return rotate_bits_left_flag_zero_opcode(memory, &memory->af.high);
}

static uint8_t opcode_0x28(CPU_Memory* memory) {
	return shift_bits_right_opcode(memory, &memory->bc.high);
}

static uint8_t opcode_0x29(CPU_Memory* memory) {
	return shift_bits_right_opcode(memory, &memory->bc.low);
}

static uint8_t opcode_0x2A(CPU_Memory* memory) {
	return shift_bits_right_opcode(memory, &memory->de.high);
}

static uint8_t opcode_0x2B(CPU_Memory* memory) {
	return shift_bits_right_opcode(memory, &memory->de.low);
}

static uint8_t opcode_0x2C(CPU_Memory* memory) {
	return shift_bits_right_opcode(memory, &memory->hl.high);
}

static uint8_t opcode_0x2D(CPU_Memory* memory) {
	return shift_bits_right_opcode(memory, &memory->hl.low);
}

static uint8_t opcode_0x2E(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	shift_bits_right_opcode(memory, &value);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0x2F(CPU_Memory* memory) {
	return shift_bits_right_opcode(memory, &memory->af.high);
}

static uint8_t opcode_0x30(CPU_Memory* memory) {
	return swap_nibbles_opcode(memory, &memory->bc.high);
}

static uint8_t opcode_0x31(CPU_Memory* memory) {
	return swap_nibbles_opcode(memory, &memory->bc.low);
}

static uint8_t opcode_0x32(CPU_Memory* memory) {
	return swap_nibbles_opcode(memory, &memory->de.high);
}

static uint8_t opcode_0x33(CPU_Memory* memory) {
	return swap_nibbles_opcode(memory, &memory->de.low);
}

static uint8_t opcode_0x34(CPU_Memory* memory) {
	return swap_nibbles_opcode(memory, &memory->hl.high);
}

static uint8_t opcode_0x35(CPU_Memory* memory) {
	return swap_nibbles_opcode(memory, &memory->hl.low);
}

static uint8_t opcode_0x36(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->hl.value);

	swap_nibbles_opcode(memory, &value);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0x37(CPU_Memory* memory) {
	return swap_nibbles_opcode(memory, &memory->af.high);
}

static uint8_t opcode_0x38(CPU_Memory* memory) {
	return shift_bits_right_logical_opcode(memory, &memory->bc.high);
}

static uint8_t opcode_0x39(CPU_Memory* memory) {
	return shift_bits_right_logical_opcode(memory, &memory->bc.low);
}

static uint8_t opcode_0x3A(CPU_Memory* memory) {
	return shift_bits_right_logical_opcode(memory, &memory->de.high);
}

static uint8_t opcode_0x3B(CPU_Memory* memory) {
	return shift_bits_right_logical_opcode(memory, &memory->de.low);
}

static uint8_t opcode_0x3C(CPU_Memory* memory) {
	return shift_bits_right_logical_opcode(memory, &memory->hl.high);
}

static uint8_t opcode_0x3D(CPU_Memory* memory) {
	return shift_bits_right_logical_opcode(memory, &memory->hl.low);
}

static uint8_t opcode_0x3E(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->hl.value);

	shift_bits_right_logical_opcode(memory, &value);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0x3F(CPU_Memory* memory) {
	return shift_bits_right_logical_opcode(memory, &memory->af.high);
}

static uint8_t opcode_0x40(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.high, 0);
}

static uint8_t opcode_0x41(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.low, 0);
}

static uint8_t opcode_0x42(CPU_Memory* memory) {
	return bit_check(memory, memory->de.high, 0);
}

static uint8_t opcode_0x43(CPU_Memory* memory) {
	return bit_check(memory, memory->de.low, 0);
}

static uint8_t opcode_0x44(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.high, 0);
}

static uint8_t opcode_0x45(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.low, 0);
}

static uint8_t opcode_0x46(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	bit_check(memory, value, 0);

	return 12;
}

static uint8_t opcode_0x47(CPU_Memory* memory) {
	return bit_check(memory, memory->af.high, 0);
}

static uint8_t opcode_0x48(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.high, 1);
}

static uint8_t opcode_0x49(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.low, 1);
}

static uint8_t opcode_0x4A(CPU_Memory* memory) {
	return bit_check(memory, memory->de.high, 1);
}

static uint8_t opcode_0x4B(CPU_Memory* memory) {
	return bit_check(memory, memory->de.low, 1);
}

static uint8_t opcode_0x4C(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.high, 1);
}

static uint8_t opcode_0x4D(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.low, 1);
}

static uint8_t opcode_0x4E(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->hl.value);

	bit_check(memory, value, 1);

	return 12;
}

static uint8_t opcode_0x4F(CPU_Memory* memory) {
	return bit_check(memory, memory->af.high, 1);
}

static uint8_t opcode_0x50(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.high, 2);
}

static uint8_t opcode_0x51(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.low, 2);
}

static uint8_t opcode_0x52(CPU_Memory* memory) {
	return bit_check(memory, memory->de.high, 2);
}

static uint8_t opcode_0x53(CPU_Memory* memory) {
	return bit_check(memory, memory->de.low, 2);
}

static uint8_t opcode_0x54(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.high, 2);
}

static uint8_t opcode_0x55(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.low, 2);
}

static uint8_t opcode_0x56(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	bit_check(memory, value, 2);

	return 12;
}

static uint8_t opcode_0x57(CPU_Memory* memory) {
	return bit_check(memory, memory->af.high, 2);
}

static uint8_t opcode_0x58(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.high, 3);
}

static uint8_t opcode_0x59(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.low, 3);
}

static uint8_t opcode_0x5A(CPU_Memory* memory) {
	return bit_check(memory, memory->de.high, 3);
}

static uint8_t opcode_0x5B(CPU_Memory* memory) {
	return bit_check(memory, memory->de.low, 3);
}

static uint8_t opcode_0x5C(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.high, 3);
}

static uint8_t opcode_0x5D(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.low, 3);
}

static uint8_t opcode_0x5E(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	bit_check(memory, value, 3);

	return 12;
}

static uint8_t opcode_0x5F(CPU_Memory* memory) {
	return bit_check(memory, memory->af.high, 3);
}

static uint8_t opcode_0x60(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.high, 4);
}

static uint8_t opcode_0x61(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.low, 4);
}

static uint8_t opcode_0x62(CPU_Memory* memory) {
	return bit_check(memory, memory->de.high, 4);
}

static uint8_t opcode_0x63(CPU_Memory* memory) {
	return bit_check(memory, memory->de.low, 4);
}

static uint8_t opcode_0x64(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.high, 4);
}

static uint8_t opcode_0x65(CPU_Memory* memory)  {
	return bit_check(memory, memory->hl.low, 4);
}

static uint8_t opcode_0x66(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	bit_check(memory, value, 4);

	return 12;
}

static uint8_t opcode_0x67(CPU_Memory* memory) {
	return bit_check(memory, memory->af.high, 4);
}

static uint8_t opcode_0x68(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.high, 5);
}

static uint8_t opcode_0x69(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.low, 5);
}

static uint8_t opcode_0x6A(CPU_Memory* memory) {
	return bit_check(memory, memory->de.high, 5);
}

static uint8_t opcode_0x6B(CPU_Memory* memory) {
	return bit_check(memory, memory->de.low , 5);
}

static uint8_t opcode_0x6C(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.high, 5);
}

static uint8_t opcode_0x6D(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.low, 5);
}

static uint8_t opcode_0x6E(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	bit_check(memory, value, 5);

	return 12;
}

static uint8_t opcode_0x6F(CPU_Memory* memory) {
	return bit_check(memory, memory->af.high, 5);
}

static uint8_t opcode_0x70(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.high, 6);
}

static uint8_t opcode_0x71(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.low, 6);
}

static uint8_t opcode_0x72(CPU_Memory* memory) {
	return bit_check(memory, memory->de.high, 6);
}

static uint8_t opcode_0x73(CPU_Memory* memory) {
	return bit_check(memory, memory->de.low, 6);
}

static uint8_t opcode_0x74(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.high, 6);
}

static uint8_t opcode_0x75(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.low, 6);
}

static uint8_t opcode_0x76(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->hl.value);

	bit_check(memory, value, 6);

	return 12;
}

static uint8_t opcode_0x77(CPU_Memory* memory) {
	return bit_check(memory, memory->af.high, 6);
}

static uint8_t opcode_0x78(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.high, 7);
}

static uint8_t opcode_0x79(CPU_Memory* memory) {
	return bit_check(memory, memory->bc.low, 7);
}

static uint8_t opcode_0x7A(CPU_Memory* memory) {
	return bit_check(memory, memory->de.high, 7);
}

static uint8_t opcode_0x7B(CPU_Memory* memory) {
	return bit_check(memory, memory->de.low, 7);
}

static uint8_t opcode_0x7C(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.high, 7);
}

static uint8_t opcode_0x7D(CPU_Memory* memory) {
	return bit_check(memory, memory->hl.low, 7);
}

static uint8_t opcode_0x7E(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->hl.value);

	bit_check(memory, value, 7);

	return 12;
}

static uint8_t opcode_0x7F(CPU_Memory* memory) {
	return bit_check(memory, memory->af.high, 7);
}

static uint8_t opcode_0x80(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.high, 0);
}

static uint8_t opcode_0x81(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.low, 0);
}

static uint8_t opcode_0x82(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.high, 0);
}

static uint8_t opcode_0x83(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.low, 0);
}

static uint8_t opcode_0x84(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.high, 0);
}

static uint8_t opcode_0x85(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.low, 0);
}

static uint8_t opcode_0x86(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	set_bit_to_zero(memory, &value, 0);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0x87(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->af.high, 0);
}

static uint8_t opcode_0x88(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.high, 1);
}

static uint8_t opcode_0x89(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.low, 1);
}

static uint8_t opcode_0x8A(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.high, 1);
}

static uint8_t opcode_0x8B(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.low, 1);
}

static uint8_t opcode_0x8C(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.high, 1);
}

static uint8_t opcode_0x8D(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.low, 1);
}

static uint8_t opcode_0x8E(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	set_bit_to_zero(memory, &value, 1);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0x8F(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->af.high, 1);
}

static uint8_t opcode_0x90(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.high, 2);
}

static uint8_t opcode_0x91(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.low, 2);
}

static uint8_t opcode_0x92(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.high, 2);
}

static uint8_t opcode_0x93(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.low, 2);
}

static uint8_t opcode_0x94(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.high, 2);
}

static uint8_t opcode_0x95(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.low, 2);
}

static uint8_t opcode_0x96(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	set_bit_to_zero(memory, &value, 2);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0x97(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->af.high, 2);
}

static uint8_t opcode_0x98(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.high, 3);
}

static uint8_t opcode_0x99(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.low, 3);
}

static uint8_t opcode_0x9A(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.high, 3);
}

static uint8_t opcode_0x9B(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.low, 3);
}

static uint8_t opcode_0x9C(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.high, 3);
}

static uint8_t opcode_0x9D(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.low, 3);
}

static uint8_t opcode_0x9E(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	set_bit_to_zero(memory, &value, 3);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0x9F(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->af.high, 3);
}

static uint8_t opcode_0xA0(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.high, 4);
}

static uint8_t opcode_0xA1(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.low, 4);
}

static uint8_t opcode_0xA2(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.high, 4);
}

static uint8_t opcode_0xA3(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.low, 4);
}

static uint8_t opcode_0xA4(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.high, 4);
}

static uint8_t opcode_0xA5(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.low, 4);
}

static uint8_t opcode_0xA6(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	set_bit_to_zero(memory, &value, 4);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0xA7(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->af.high, 4);
}

static uint8_t opcode_0xA8(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.high, 5);
}

static uint8_t opcode_0xA9(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.low, 5);
}

static uint8_t opcode_0xAA(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.high, 5);
}

static uint8_t opcode_0xAB(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.low, 5);
}

static uint8_t opcode_0xAC(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.high, 5);
}

static uint8_t opcode_0xAD(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.low, 5);
}

static uint8_t opcode_0xAE(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	set_bit_to_zero(memory, &value, 5);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0xAF(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->af.high, 5);
}

static uint8_t opcode_0xB0(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.high, 6);
}

static uint8_t opcode_0xB1(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.low, 6);
}

static uint8_t opcode_0xB2(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.high, 6);
}

static uint8_t opcode_0xB3(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.low, 6);
}

static uint8_t opcode_0xB4(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.high, 6);
}

static uint8_t opcode_0xB5(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.low, 6);
}

static uint8_t opcode_0xB6(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	set_bit_to_zero(memory, &value, 6);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0xB7(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->af.high, 6);
}

static uint8_t opcode_0xB8(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.high, 7);
}

static uint8_t opcode_0xB9(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->bc.low, 7);
}

static uint8_t opcode_0xBA(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.high, 7);
}

static uint8_t opcode_0xBB(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->de.low, 7);
}

static uint8_t opcode_0xBC(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.high, 7);
}

static uint8_t opcode_0xBD(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->hl.low, 7);
}

static uint8_t opcode_0xBE(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->hl.value);

	set_bit_to_zero(memory, &value, 7);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0xBF(CPU_Memory* memory) {
	return set_bit_to_zero(memory, &memory->af.high, 7);
}

static uint8_t opcode_0xC0(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->bc.high, 0);
}

static uint8_t opcode_0xC1(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->bc.low, 0);
}

static uint8_t opcode_0xC2(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->de.high, 0);
}

static uint8_t opcode_0xC3(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->de.low, 0);
}

static uint8_t opcode_0xC4(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->hl.high, 0);
}

static uint8_t opcode_0xC5(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->hl.low, 0);
}

static uint8_t opcode_0xC6(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->hl.value);

	set_bit_to_one(memory, &value, 0);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0xC7(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->af.high, 0);
}

static uint8_t opcode_0xC8(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->bc.high, 1);
}

static uint8_t opcode_0xC9(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->bc.low, 1);
}

static uint8_t opcode_0xCA(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->de.high, 1);
}

static uint8_t opcode_0xCB(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->de.low, 1);
}

static uint8_t opcode_0xCC(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->hl.high, 1);
}

static uint8_t opcode_0xCD(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->hl.low, 1);
}

static uint8_t opcode_0xCE(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->hl.value);

	set_bit_to_one(memory, &value, 1);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0xCF(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->af.high, 1);
}

static uint8_t opcode_0xD0(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->bc.high, 2);
}

static uint8_t opcode_0xD1(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->bc.low, 2);
}

static uint8_t opcode_0xD2(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->de.high, 2);
}

static uint8_t opcode_0xD3(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->de.low, 2);
}

static uint8_t opcode_0xD4(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->hl.high, 2);
}

static uint8_t opcode_0xD5(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->hl.low, 2);
}

static uint8_t opcode_0xD6(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->hl.value);

	set_bit_to_one(memory, &value, 2);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0xD7(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->af.high, 2);
}

static uint8_t opcode_0xD8(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->bc.high, 3);
}

static uint8_t opcode_0xD9(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->bc.low, 3);
}

static uint8_t opcode_0xDA(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->de.high, 3);
}

static uint8_t opcode_0xDB(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->de.low, 3);
}

static uint8_t opcode_0xDC(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->hl.high, 3);
}

static uint8_t opcode_0xDD(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->hl.low, 3);
}

static uint8_t opcode_0xDE(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	set_bit_to_one(memory, &value, 3);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0xDF(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->af.high, 3);
}

static uint8_t opcode_0xE0(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->bc.high, 4);
}

static uint8_t opcode_0xE1(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->bc.low, 4);
}

static uint8_t opcode_0xE2(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->de.high, 4);
}

static uint8_t opcode_0xE3(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->de.low, 4);
}

static uint8_t opcode_0xE4(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->hl.high, 4);
}

static uint8_t opcode_0xE5(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->hl.low, 4);
}

static uint8_t opcode_0xE6(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->hl.value);

	set_bit_to_one(memory, &value, 4);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0xE7(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->af.high, 4);
}

static uint8_t opcode_0xE8(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->bc.high,5);
}

static uint8_t opcode_0xE9(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->bc.low, 5);
}

static uint8_t opcode_0xEA(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->de.high, 5);
}

static uint8_t opcode_0xEB(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->de.low, 5);
}

static uint8_t opcode_0xEC(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->hl.high, 5);
}

static uint8_t opcode_0xED(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->hl.low, 5);
}

static uint8_t opcode_0xEE(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->hl.value);

	set_bit_to_one(memory, &value, 5);
	memory_write(memory, memory->hl.value, value);

	return 16;
}

static uint8_t opcode_0xEF(CPU_Memory* memory) {
	return set_bit_to_one(memory, &memory->af.high, 5);
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
		case 0x20:
			cycles = opcode_0x20(memory);
			break;
		case 0x21:
			cycles = opcode_0x21(memory);
			break;
		case 0x22:
			cycles = opcode_0x22(memory);
			break;
		case 0x23:
			cycles = opcode_0x23(memory);
			break;
		case 0x24:
			cycles = opcode_0x24(memory);
			break;
		case 0x25:
			cycles = opcode_0x25(memory);
			break;
		case 0x26:
			cycles = opcode_0x26(memory);
			break;
		case 0x27:
			cycles = opcode_0x27(memory);
			break;
		case 0x28:
			cycles = opcode_0x28(memory);
			break;
		case 0x29:
			cycles = opcode_0x29(memory);
			break;
		case 0x2A:
			cycles = opcode_0x2A(memory);
			break;
		case 0x2B:
			cycles = opcode_0x2B(memory);
			break;
		case 0x2C:
			cycles = opcode_0x2C(memory);
			break;
		case 0x2D:
			cycles = opcode_0x2D(memory);
			break;
		case 0x2E:
			cycles = opcode_0x2E(memory);
			break;
		case 0x2F:
			cycles = opcode_0x2F(memory);
			break;
		case 0x30:
			cycles = opcode_0x30(memory);
			break;
		case 0x31:
			cycles = opcode_0x31(memory);
			break;
		case 0x32:
			cycles = opcode_0x32(memory);
			break;
		case 0x33:
			cycles = opcode_0x33(memory);
			break;
		case 0x34:
			cycles = opcode_0x34(memory);
			break;
		case 0x35:
			cycles = opcode_0x35(memory);
			break;
		case 0x36:
			cycles = opcode_0x36(memory);
			break;
		case 0x37:
			cycles = opcode_0x37(memory);
			break;
		case 0x38:
			cycles = opcode_0x38(memory);
			break;
		case 0x39:
			cycles = opcode_0x39(memory);
			break;
		case 0x3A:
			cycles = opcode_0x3A(memory);
			break;
		case 0x3B:
			cycles = opcode_0x3B(memory);
			break;
		case 0x3C:
			cycles = opcode_0x3C(memory);
			break;
		case 0x3D:
			cycles = opcode_0x3D(memory);
			break;
		case 0x3E:
			cycles = opcode_0x3E(memory);
			break;
		case 0x3F:
			cycles = opcode_0x3F(memory);
			break;
		case 0x40:
			cycles = opcode_0x40(memory);
			break;
		case 0x41:
			cycles = opcode_0x41(memory);
			break;
		case 0x42:
			cycles = opcode_0x42(memory);
			break;
		case 0x43:
			cycles = opcode_0x43(memory);
			break;
		case 0x44:
			cycles = opcode_0x44(memory);
			break;
		case 0x45:
			cycles = opcode_0x45(memory);
			break;
		case 0x46:
			cycles = opcode_0x46(memory);
			break;
		case 0x47:
			cycles = opcode_0x47(memory);
			break;
		case 0x48:
			cycles = opcode_0x48(memory);
			break;
		case 0x49:
			cycles = opcode_0x49(memory);
			break;
		case 0x4A:
			cycles = opcode_0x4A(memory);
			break;
		case 0x4B:
			cycles = opcode_0x4B(memory);
			break;
		case 0x4C:
			cycles = opcode_0x4C(memory);
			break;
		case 0x4D:
			cycles = opcode_0x4D(memory);
			break;
		case 0x4E:
			cycles = opcode_0x4E(memory);
			break;
		case 0x4F:
			cycles = opcode_0x4F(memory);
			break;
		case 0x50:
			cycles = opcode_0x50(memory);
			break;
		case 0x51:
			cycles = opcode_0x51(memory);
			break;
		case 0x52:
			cycles = opcode_0x52(memory);
			break;
		case 0x53:
			cycles = opcode_0x53(memory);
			break;
		case 0x54:
			cycles = opcode_0x54(memory);
			break;
		case 0x55:
			cycles = opcode_0x55(memory);
			break;
		case 0x56:
			cycles = opcode_0x56(memory);
			break;
		case 0x57:
			cycles = opcode_0x57(memory);
			break;
		case 0x58:
			cycles = opcode_0x58(memory);
			break;
		case 0x59:
			cycles = opcode_0x59(memory);
			break;
		case 0x5A:
			cycles = opcode_0x5A(memory);
			break;
		case 0x5B:
			cycles = opcode_0x5B(memory);
			break;
		case 0x5C:
			cycles = opcode_0x5C(memory);
			break;
		case 0x5D:
			cycles = opcode_0x5D(memory);
			break;
		case 0x5E:
			cycles = opcode_0x5E(memory);
			break;
		case 0x5F:
			cycles = opcode_0x5F(memory);
			break;
		case 0x60:
			cycles = opcode_0x60(memory);
			break;
		case 0x61:
			cycles = opcode_0x61(memory);
			break;
		case 0x62:
			cycles = opcode_0x62(memory);
			break;
		case 0x63:
			cycles = opcode_0x63(memory);
			break;
		case 0x64:
			cycles = opcode_0x64(memory);
			break;
		case 0x65:
			cycles = opcode_0x65(memory);
			break;
		case 0x66:
			cycles = opcode_0x66(memory);
			break;
		case 0x67:
			cycles = opcode_0x67(memory);
			break;
		case 0x68:
			cycles = opcode_0x68(memory);
			break;
		case 0x69:
			cycles = opcode_0x69(memory);
			break;
		case 0x6A:
			cycles = opcode_0x6A(memory);
			break;
		case 0x6B:
			cycles = opcode_0x6B(memory);
			break;
		case 0x6C:
			cycles = opcode_0x6C(memory);
			break;
		case 0x6D:
			cycles = opcode_0x6D(memory);
			break;
		case 0x6E:
			cycles = opcode_0x6E(memory);
			break;
		case 0x6F:
			cycles = opcode_0x6F(memory);
			break;
		case 0x70:
			cycles = opcode_0x70(memory);
			break;
		case 0x71:
			cycles = opcode_0x71(memory);
			break;
		case 0x72:
			cycles = opcode_0x72(memory);
			break;
		case 0x73:
			cycles = opcode_0x73(memory);
			break;
		case 0x74:
			cycles = opcode_0x74(memory);
			break;
		case 0x75:
			cycles = opcode_0x75(memory);
			break;
		case 0x76:
			cycles = opcode_0x76(memory);
			break;
		case 0x77:
			cycles = opcode_0x77(memory);
			break;
		case 0x78:
			cycles = opcode_0x78(memory);
			break;
		case 0x79:
			cycles = opcode_0x79(memory);
			break;
		case 0x7A:
			cycles = opcode_0x7A(memory);
			break;
		case 0x7B:
			cycles = opcode_0x7B(memory);
			break;
		case 0x7C:
			cycles = opcode_0x7C(memory);
			break;
		case 0x7D:
			cycles = opcode_0x7D(memory);
			break;
		case 0x7E:
			cycles = opcode_0x7E(memory);
			break;
		case 0x7F:
			cycles = opcode_0x7F(memory);
			break;
		case 0x80:
			cycles = opcode_0x80(memory);
			break;
		case 0x81:
			cycles = opcode_0x81(memory);
			break;
		case 0x82:
			cycles = opcode_0x82(memory);
			break;
		case 0x83:
			cycles = opcode_0x83(memory);
			break;
		case 0x84:
			cycles = opcode_0x84(memory);
			break;
		case 0x85:
			cycles = opcode_0x85(memory);
			break;
		case 0x86:
			cycles = opcode_0x86(memory);
			break;
		case 0x87:
			cycles = opcode_0x87(memory);
			break;
		case 0x88:
			cycles = opcode_0x88(memory);
			break;
		case 0x89:
			cycles = opcode_0x89(memory);
			break;
		case 0x8A:
			cycles = opcode_0x8A(memory);
			break;
		case 0x8B:
			cycles = opcode_0x8B(memory);
			break;
		case 0x8C:
			cycles = opcode_0x8C(memory);
			break;
		case 0x8D:
			cycles = opcode_0x8D(memory);
			break;
		case 0x8E:
			cycles = opcode_0x8E(memory);
			break;
		case 0x8F:
			cycles = opcode_0x8F(memory);
			break;
		case 0x90:
			cycles = opcode_0x90(memory);
			break;
		case 0x91:
			cycles = opcode_0x91(memory);
			break;
		case 0x92:
			cycles = opcode_0x92(memory);
			break;
		case 0x93:
			cycles = opcode_0x93(memory);
			break;
		case 0x94:
			cycles = opcode_0x94(memory);
			break;
		case 0x95:
			cycles = opcode_0x95(memory);
			break;
		case 0x96:
			cycles = opcode_0x96(memory);
			break;
		case 0x97:
			cycles = opcode_0x97(memory);
			break;
		case 0x98:
			cycles = opcode_0x98(memory);
			break;
		case 0x99:
			cycles = opcode_0x99(memory);
			break;
		case 0x9A:
			cycles = opcode_0x9A(memory);
			break;
		case 0x9B:
			cycles = opcode_0x9B(memory);
			break;
		case 0x9C:
			cycles = opcode_0x9C(memory);
			break;
		case 0x9D:
			cycles = opcode_0x9D(memory);
			break;
		case 0x9E:
			cycles = opcode_0x9E(memory);
			break;
		case 0x9F:
			cycles = opcode_0x9F(memory);
			break;
		case 0xA0:
			cycles = opcode_0xA0(memory);
			break;
		case 0xA1:
			cycles = opcode_0xA1(memory);
			break;
		case 0xA2:
			cycles = opcode_0xA2(memory);
			break;
		case 0xA3:
			cycles = opcode_0xA3(memory);
			break;
		case 0xA4:
			cycles = opcode_0xA4(memory);
			break;
		case 0xA5:
			cycles = opcode_0xA5(memory);
			break;
		case 0xA6:
			cycles = opcode_0xA6(memory);
			break;
		case 0xA7:
			cycles = opcode_0xA7(memory);
			break;
		case 0xA8:
			cycles = opcode_0xA8(memory);
			break;
		case 0xA9:
			cycles = opcode_0xA9(memory);
			break;
		case 0xAA:
			cycles = opcode_0xAA(memory);
			break;
		case 0xAB:
			cycles = opcode_0xAB(memory);
			break;
		case 0xAC:
			cycles = opcode_0xAC(memory);
			break;
		case 0xAD:
			cycles = opcode_0xAD(memory);
			break;
		case 0xAE:
			cycles = opcode_0xAE(memory);
			break;
		case 0xAF:
			cycles = opcode_0xAF(memory);
			break;
		case 0xB0:
			cycles = opcode_0xB0(memory);
			break;
		case 0xB1:
			cycles = opcode_0xB1(memory);
			break;
		case 0xB2:
			cycles = opcode_0xB2(memory);
			break;
		case 0xB3:
			cycles = opcode_0xB3(memory);
			break;
		case 0xB4:
			cycles = opcode_0xB4(memory);
			break;
		case 0xB5:
			cycles = opcode_0xB5(memory);
			break;
		case 0xB6:
			cycles = opcode_0xB6(memory);
			break;
		case 0xB7:
			cycles = opcode_0xB7(memory);
			break;
		case 0xB8:
			cycles = opcode_0xB8(memory);
			break;
		case 0xB9:
			cycles = opcode_0xB9(memory);
			break;
		case 0xBA:
			cycles = opcode_0xBA(memory);
			break;
		case 0xBB:
			cycles = opcode_0xBB(memory);
			break;
		case 0xBC:
			cycles = opcode_0xBC(memory);
			break;
		case 0xBD:
			cycles = opcode_0xBD(memory);
			break;
		case 0xBE:
			cycles = opcode_0xBE(memory);
			break;
		case 0xBF:
			cycles = opcode_0xBF(memory);
			break;
		case 0xC0:
			cycles = opcode_0xC0(memory);
			break;
		case 0xC1:
			cycles = opcode_0xC1(memory);
			break;
		case 0xC2:
			cycles = opcode_0xC2(memory);
			break;
		case 0xC3:
			cycles = opcode_0xC3(memory);
			break;
		case 0xC4:
			cycles = opcode_0xC4(memory);
			break;
		case 0xC5:
			cycles = opcode_0xC5(memory);
			break;
		case 0xC6:
			cycles = opcode_0xC6(memory);
			break;
		case 0xC7:
			cycles = opcode_0xC7(memory);
			break;
		case 0xC8:
			cycles = opcode_0xC8(memory);
			break;
		case 0xC9:
			cycles = opcode_0xC9(memory);
			break;
		case 0xCA:
			cycles = opcode_0xCA(memory);
			break;
		case 0xCB:
			cycles = opcode_0xCB(memory);
			break;
		case 0xCC:
			cycles = opcode_0xCC(memory);
			break;
		case 0xCD:
			cycles = opcode_0xCD(memory);
			break;
		case 0xCE:
			cycles = opcode_0xCE(memory);
			break;
		case 0xCF:
			cycles = opcode_0xCF(memory);
			break;
		case 0xD0:
			cycles = opcode_0xD0(memory);
			break;
		case 0xD1:
			cycles = opcode_0xD1(memory);
			break;
		case 0xD2:
			cycles = opcode_0xD2(memory);
			break;
		case 0xD3:
			cycles = opcode_0xD3(memory);
			break;
		case 0xD4:
			cycles = opcode_0xD4(memory);
			break;
		case 0xD5:
			cycles = opcode_0xD5(memory);
			break;
		case 0xD6:
			cycles = opcode_0xD6(memory);
			break;
		case 0xD7:
			cycles = opcode_0xD7(memory);
			break;
		case 0xD8:
			cycles = opcode_0xD8(memory);
			break;
		case 0xD9:
			cycles = opcode_0xD9(memory);
			break;
		case 0xDA:
			cycles = opcode_0xDA(memory);
			break;
		case 0xDB:
			cycles = opcode_0xDB(memory);
			break;
		case 0xDC:
			cycles = opcode_0xDC(memory);
			break;
		case 0xDD:
			cycles = opcode_0xDD(memory);
			break;
		case 0xDE:
			cycles = opcode_0xDE(memory);
			break;
		case 0xDF:
			cycles = opcode_0xDF(memory);
			break;
		case 0xE0:
			cycles = opcode_0xE0(memory);
			break;
		case 0xE1:
			cycles = opcode_0xE1(memory);
			break;
		case 0xE2:
			cycles = opcode_0xE2(memory);
			break;
		case 0xE3:
			cycles = opcode_0xE3(memory);
			break;
		case 0xE4:
			cycles = opcode_0xE4(memory);
			break;
		case 0xE5:
			cycles = opcode_0xE5(memory);
			break;
		case 0xE6:
			cycles = opcode_0xE6(memory);
			break;
		case 0xE7:
			cycles = opcode_0xE7(memory);
			break;
		case 0xE8:
			cycles = opcode_0xE8(memory);
			break;
		case 0xE9:
			cycles = opcode_0xE9(memory);
			break;
		case 0xEA:
			cycles = opcode_0xEA(memory);
			break;
		case 0xEB:
			cycles = opcode_0xEB(memory);
			break;
		case 0xEC:
			cycles = opcode_0xEC(memory);
			break;
		case 0xED:
			cycles = opcode_0xED(memory);
			break;
		case 0xEE:
			cycles = opcode_0xEE(memory);
			break;
		case 0xEF:
			cycles = opcode_0xEF(memory);
			break;
		default:
			break;
	}

	return cycles;
}