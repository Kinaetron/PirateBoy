#include "cpu.h"
#include "memory.h"
#include <stdbool.h>

static uint8_t memory_read(CPU_Memory* memory, uint16_t address)
{
	if (address >= ROM_START && address <= ROM_END) {
		return memory->rom[address];
	}
	else if (address >= WRAM_START && address <= WRAM_END) {
		return memory->wram[address - WRAM_START];
	}
	else if (address >= ECHO_RAM_START && address <= ECHO_RAM_END) {
		return memory->wram[address - ECHO_RAM_START];
	}
	else if (address >= IO_START && address <= IO_END) {
		return memory->input_output[address - IO_START];
	}
	else if (address >= HRAM_START && address <= HRAM_END) {
		return memory->hram[address - HRAM_START];
	}
	else if (address == INTERRUPT_ENABLE_ADDR) {
		return memory->interrupt_enable;
	}

	return MMU_UNMAPPED_READ_VALUE;
}

static void memory_write(CPU_Memory* memory, uint16_t address, uint8_t data)
{
	if (address >= ROM_START && address <= ROM_END) {
		return;
	}
	else if (address >= WRAM_START && address <= WRAM_END) {
		memory->wram[address - WRAM_START] = data;
	}
	else if (address >= ECHO_RAM_START && address <= ECHO_RAM_END) {
		memory->wram[address - ECHO_RAM_START] = data;
	}
	else if (address >= IO_START && address <= IO_END) {
		memory->input_output[address - IO_START] = data;
	}
	else if (address >= HRAM_START && address <= HRAM_END) {
		memory->hram[address - HRAM_START] = data;
	}
	else if (address == INTERRUPT_ENABLE_ADDR) {
		memory->interrupt_enable = data;
	}
	else 
	{
#ifdef DEBUG
		fprintf(stderr, "Unmapped write: addr=0x%04X data=0x%02X\n", address, data);
#endif
	}
}

static bool get_register_flag(CPU_Memory* memory, Flag flag) {
	return (memory->af.low >> flag) & 1;
}

static void set_register_flag(CPU_Memory* memory, Flag flag, bool value)
{
	if (value) {
		memory->af.low |= (1 << flag);
	}
	else {
		memory->af.low &= ~(1 << flag);
	}

	memory->af.low &= 0xF0;
}

static uint8_t fetch_byte(CPU_Memory* memory)
{
	uint8_t value = memory_read(memory, memory->program_counter);
	memory->program_counter++;
	return value;
}

static memory16 fetch_two_bytes(CPU_Memory* memory)
{
	memory16 result;
	result.low = fetch_byte(memory);
	result.high = fetch_byte(memory);

	return result;
}

static uint8_t opcode_0x01(CPU_Memory* memory)
{
	memory->bc.value = fetch_two_bytes(memory).value;

	return 12;
}

static uint8_t opcode_0x02(CPU_Memory* memory)
{
	uint8_t a_register = memory->af.high;
	memory_write(memory, memory->bc.value, a_register);

	return 8;
}

static uint8_t opcode_0x03(CPU_Memory* memory) 
{
	memory->bc.value++;

	return 8;
}

static uint8_t opcode_0x04(CPU_Memory* memory)
{
	set_register_flag(memory, H, (memory->bc.high & 0x0F) == 0x0F);
	set_register_flag(memory, N, false);

	memory->bc.high++;

	set_register_flag(memory, Z, memory->bc.high == 0x00);

	return 4;
}

static uint8_t opcode_0x05(CPU_Memory* memory)
{
	set_register_flag(memory, H, (memory->bc.high & 0x0F) == 0x00);
	set_register_flag(memory, N, true);

	memory->bc.high--;

	set_register_flag(memory, Z, memory->bc.high == 0x00);

	return 4;
}

static uint8_t opcode_0x06(CPU_Memory* memory)
{
	uint8_t high_byte = fetch_byte(memory);
	memory->bc.high = high_byte;

	return 8;
}

static uint8_t opcode_0x07(CPU_Memory* memory)
{
	uint8_t bit7 = (memory->af.high >> 7) & 1;
	memory->af.high = (memory->af.high << 1) | bit7;

	set_register_flag(memory, Z, false);
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, false);
	set_register_flag(memory, C, bit7);

	return 4;
}

static uint8_t opcode_0x08(CPU_Memory* memory)
{
	memory16 address = fetch_two_bytes(memory);
	memory16 stack_pointer;
	stack_pointer.value = memory->stack_pointer;

	memory_write(memory, address.value, stack_pointer.low);
	memory_write(memory, address.value + 1, stack_pointer.high);

	return 20;
}

static uint8_t opcode_0x09(CPU_Memory* memory)
{
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, ((memory->hl.value & 0x0FFF) + (memory->bc.value & 0x0FFF)) > 0x0FFF);
	set_register_flag(memory, C, ((uint32_t)memory->hl.value + (uint32_t)memory->bc.value) > 0xFFFF);

	memory->hl.value += memory->bc.value;

	return 8;
}

static uint8_t opcode_0x0A(CPU_Memory* memory)
{
	memory->af.high = memory_read(memory, memory->bc.value);

	return 8;
}

static uint8_t opcode_0x0B(CPU_Memory* memory)
{
	memory->bc.value--;

	return 8;
}

static uint8_t opcode_0x0C(CPU_Memory* memory)
{
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, (memory->bc.low & 0x0F) == 0x0F);

	memory->bc.low++;

	set_register_flag(memory, Z, memory->bc.low == 0x00);

	return 4;
}

static uint8_t opcode_0x0D(CPU_Memory* memory)
{
	set_register_flag(memory, N, true);
	set_register_flag(memory, H, (memory->bc.low & 0x0F) == 0x00);

	memory->bc.low--;

	set_register_flag(memory, Z, memory->bc.low == 0x00);

	return 4;
}

static uint8_t opcode_0x0E(CPU_Memory* memory)
{
	memory->bc.low = fetch_byte(memory);

	return 8;
}

static uint8_t opcode_0x0F(CPU_Memory* memory)
{
	uint8_t bit0 = memory->af.high & 0x01;
	memory->af.high = (memory->af.high >> 1) | (bit0 << 7);

	set_register_flag(memory, Z, false);
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, false);
	set_register_flag(memory, C, bit0);

	return 4;
}

static uint8_t opcode_0x11(CPU_Memory* memory)
{
	memory->de.value = fetch_two_bytes(memory).value;
	return 12;
}

static uint8_t opcode_0x12(CPU_Memory* memory)
{
	memory_write(memory, memory->de.value, memory->af.high);

	return 8;
}

static uint8_t opcode_0x13(CPU_Memory* memory)
{
	memory->de.value++;

	return 8;
}

static uint8_t opcode_0x14(CPU_Memory* memory)
{
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, (memory->de.high & 0x0F) == 0x0F);

	memory->de.high++;

	set_register_flag(memory, Z, memory->de.high == 0x00);

	return 4;
}

static uint8_t opcode_0x15(CPU_Memory* memory)
{
	set_register_flag(memory, N, true);
	set_register_flag(memory, H, (memory->de.high & 0x0F) == 0x00);

	memory->de.high--;

	set_register_flag(memory, Z, memory->de.high == 0x00);

	return 4;
}

static uint8_t opcode_0x16(CPU_Memory* memory)
{
	memory->de.high = fetch_byte(memory);

	return 8;
}

static uint8_t opcode_0x17(CPU_Memory* memory)
{
	uint8_t old_carry = get_register_flag(memory, C);
	uint8_t bit7 = (memory->af.high >> 7) & 1;

	memory->af.high = (memory->af.high << 1) | old_carry;

	set_register_flag(memory, Z, false);
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, false);
	set_register_flag(memory, C, bit7);

	return 4;
}

static uint8_t opcode_0x18(CPU_Memory* memory)
{
	int8_t jump_value = (int8_t)fetch_byte(memory);
	memory->program_counter += jump_value;

	return 12;
}

static uint8_t opcode_0x19(CPU_Memory* memory)
{
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, ((memory->hl.value & 0x0FFF) + (memory->de.value & 0x0FFF)) > 0x0FFF);
	set_register_flag(memory, C, ((uint32_t)memory->hl.value + (uint32_t)memory->de.value) > 0xFFFF);

	memory->hl.value += memory->de.value;

	return 8;
}

static uint8_t opcode_0x1A(CPU_Memory* memory)
{
	memory->af.high = memory_read(memory, memory->de.value);

	return 8;
}

static uint8_t opcode_0x1B(CPU_Memory* memory)
{
	memory->de.value--;

	return 8;
}

static uint8_t opcode_0x1C(CPU_Memory* memory)
{
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, (memory->de.low & 0x0F) == 0x0F);

	memory->de.low++;

	set_register_flag(memory, Z, memory->de.low == 0x00);

	return 4;
}

static uint8_t opcode_0x1D(CPU_Memory* memory)
{
	set_register_flag(memory, N, true);
	set_register_flag(memory, H, (memory->de.low & 0x0F) == 0x00);

	memory->de.low--;

	set_register_flag(memory, Z, memory->de.low == 0x00);

	return 4;
}

static uint8_t opcode_0x1E(CPU_Memory* memory)
{
	memory->de.low = fetch_byte(memory);

	return 8;
}

static uint8_t opcode_0x1F(CPU_Memory* memory)
{
	uint8_t old_carry = get_register_flag(memory, C);
	uint8_t bit0 = memory->af.high & 0x01;

	memory->af.high = (memory->af.high >> 1) | (old_carry << 7);

	set_register_flag(memory, Z, false);
	set_register_flag(memory, N, false);
	set_register_flag(memory, H, false);
	set_register_flag(memory, C, bit0);

	return 4;
}

uint8_t cpu_step(CPU_Memory* memory)
{
	uint8_t opcode = fetch_byte(memory);
	uint8_t cycles = 4;

	switch (opcode)
	{
		case 0x00:
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