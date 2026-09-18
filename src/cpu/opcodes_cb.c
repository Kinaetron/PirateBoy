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

static uint8_t bit_zero_check_opcode(CPU_Memory* memory, uint8_t value) {
	return bit_check(memory, value, 0);
}

static uint8_t bit_one_check_opcode(CPU_Memory* memory, uint8_t value) {
	return bit_check(memory, value, 1);
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
	return bit_zero_check_opcode(memory, memory->bc.high);
}

static uint8_t opcode_0x41(CPU_Memory* memory) {
	return bit_zero_check_opcode(memory, memory->bc.low);
}

static uint8_t opcode_0x42(CPU_Memory* memory) {
	return bit_zero_check_opcode(memory, memory->de.high);
}

static uint8_t opcode_0x43(CPU_Memory* memory) {
	return bit_zero_check_opcode(memory, memory->de.low);
}

static uint8_t opcode_0x44(CPU_Memory* memory) {
	return bit_zero_check_opcode(memory, memory->hl.high);
}

static uint8_t opcode_0x45(CPU_Memory* memory) {
	return bit_zero_check_opcode(memory, memory->hl.low);
}

static uint8_t opcode_0x46(CPU_Memory* memory) 
{
	uint8_t value = memory_read(memory, memory->hl.value);

	bit_zero_check_opcode(memory, value);
	memory_write(memory, memory->hl.value, value);

	return 12;
}

static uint8_t opcode_0x47(CPU_Memory* memory) {
	return bit_zero_check_opcode(memory, memory->af.high);
}

static uint8_t opcode_0x48(CPU_Memory* memory) {
	return bit_one_check_opcode(memory, memory->bc.high);
}

static uint8_t opcode_0x49(CPU_Memory* memory) {
	return bit_one_check_opcode(memory, memory->bc.low);
}

static uint8_t opcode_0x4A(CPU_Memory* memory) {
	return bit_one_check_opcode(memory, memory->de.high);
}

static uint8_t opcode_0x4B(CPU_Memory* memory) {
	return bit_one_check_opcode(memory, memory->de.low);
}

static uint8_t opcode_0x4C(CPU_Memory* memory) {
	return bit_one_check_opcode(memory, memory->hl.high);
}

static uint8_t opcode_0x4D(CPU_Memory* memory) {
	return bit_one_check_opcode(memory, memory->hl.low);
}

static uint8_t opcode_0x4E(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->hl.value);

	bit_one_check_opcode(memory, value);
	memory_write(memory, memory->hl.value, value);

	return 12;
}

static uint8_t opcode_0x4F(CPU_Memory* memory) {
	return bit_one_check_opcode(memory, memory->af.high);
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
		default:
			break;
	}

	return cycles;
}