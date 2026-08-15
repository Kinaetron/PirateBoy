#include "cpu.h"
#include "unity.h"
#include "memory.h"

#include <stdlib.h>
#include <stdbool.h>

#define TEST_ROM_SIZE 32768

CPU_Memory* memory;

void setUp(void) 
{
    memory = calloc(1, sizeof(CPU_Memory));

    if (memory != NULL) 
    {
        memory->rom = malloc(TEST_ROM_SIZE);
        if (memory->rom != NULL) {
            memset(memory->rom, 0x00, TEST_ROM_SIZE);
        }
    }
}

void tearDown(void) 
{
    if (memory != NULL)
    {
        free(memory->rom);
        free(memory);
        memory = NULL;
    }
}

static void set_all_flags_true() {
    memory->af.low = 0xF0;
}

static void set_all_flags_false() {
    memory->af.low = 0x00;
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

void test_cpu_step_opcode_0x01(void)
{
    uint8_t low_byte = 0xD0;
    uint8_t high_byte = 0x9A;

    memory->rom[0x00] = 0x01;
    memory->rom[0x01] = low_byte;
    memory->rom[0x02] = high_byte;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(low_byte, memory->bc.low);
    TEST_ASSERT_EQUAL_UINT8(high_byte, memory->bc.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0003, memory->program_counter);
}

void test_cpu_step_opcode_0x02(void)
{
    memory->af.high = 0x42;
    memory->bc.value = 0xC050;
    set_all_flags_true();

    memory->rom[0x00] = 0x02;

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(0x42, memory->wram[0xC050 - WRAM_START]);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x03(void)
{
    memory->bc.value = 0x0005;
    set_all_flags_true();

    uint16_t result = 0x0006;

    memory->rom[0x00] = 0x03;

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(result, memory->bc.value);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x04(void)
{
    memory->bc.high = 0x05;
    set_all_flags_true();

    uint8_t expected = 0x06;

    memory->rom[0x00] = 0x04;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->bc.high);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x04_zero_flag(void)
{
    memory->bc.high = 0xFF;
    memory->af.low = 0x00;

    uint8_t expected = 0x00;

    memory->rom[0x00] = 0x04;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->bc.high);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x04_zero_flag_regression(void)
{
    memory->bc.high = 0x06;
    memory->af.low = 0x80;

    uint8_t expected = 0x07;

    memory->rom[0x00] = 0x04;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->bc.high);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x04_half_carry(void)
{
    memory->bc.high = 0x0F;
    memory->af.low = 0x00;

    uint8_t expected = 0x10;

    memory->rom[0x00] = 0x04;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->bc.high);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x05(void)
{
    memory->bc.high = 0x05;
    set_all_flags_true();

    uint8_t expected = 0x04;

    memory->rom[0x00] = 0x05;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->bc.high);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x05_zero_flag(void)
{
    memory->bc.high = 0x01;
    memory->af.low = 0x00;

    uint8_t expected = 0x00;

    memory->rom[0x00] = 0x05;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->bc.high);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x05_zero_flag_regression(void)
{
    memory->bc.high = 0x06;
    memory->af.low = 0x80;

    uint8_t expected = 0x05;

    memory->rom[0x00] = 0x05;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->bc.high);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x05_half_carry(void)
{
    memory->bc.high = 0x00;
    memory->af.low = 0x00;

    uint8_t expected = 0xFF;

    memory->rom[0x00] = 0x05;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->bc.high);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x06(void)
{
    uint8_t high_byte = 0x9A;

    memory->rom[0x00] = 0x06;
    memory->rom[0x01] = high_byte;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(high_byte, memory->bc.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0002, memory->program_counter);
}

void test_cpu_step_opcode_0x07(void)
{
    uint8_t expected = 0x1E;

    memory->rom[0x00] = 0x07;
    memory->af.high = 0x0F;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->af.high);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x07_flag_regression(void)
{
    uint8_t expected = 0x00;
    memory->rom[0x00] = 0x07;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x07_carry_flag(void)
{
    uint8_t expected = 0x01;
    memory->rom[0x00] = 0x07;

    memory->af.high = 0x80;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->af.high);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x08(void)
{
    uint8_t expected_low_byte = 0x0F;
    uint8_t expected_high_byte = 0xF0;

    uint16_t memory_address_1 = 0xC7A3;
    uint16_t memory_address_2 = 0xC7A4;

    memory->stack_pointer = 0xF00F;
    memory->rom[0x00] = 0x08;
    memory->rom[0x01] = 0xA3;
    memory->rom[0x02] = 0xC7;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(20, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_low_byte, memory->wram[memory_address_1 - WRAM_START]);
    TEST_ASSERT_EQUAL_UINT8(expected_high_byte, memory->wram[memory_address_2 - WRAM_START]);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0003, memory->program_counter);
}

void test_cpu_step_opcode_0x09(void)
{
    uint16_t expected_addition_result = 0xFB9F;

    memory->bc.value = 0x3A7C;
    memory->hl.value = 0xC123;
    memory->rom[0x00] = 0x09;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE (get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(expected_addition_result, memory->hl.value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x09_half_carry(void)
{
    uint16_t expected_addition_result = 0xE09F;

    memory->bc.value = 0x2A7C;
    memory->hl.value = 0xB623;
    memory->rom[0x00] = 0x09;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(expected_addition_result, memory->hl.value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x09_carry(void)
{
    uint16_t expected_addition_result = 0x0000;

    memory->bc.value = 0xFFFF;
    memory->hl.value = 0x0001;
    memory->rom[0x00] = 0x09;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(expected_addition_result, memory->hl.value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x0A(void)
{
    uint16_t address = 0xC8D4;
    uint8_t expected_value = 0xA5;

    memory->rom[0x00] = 0x0A;
    memory->bc.value = address;
    memory->wram[address - WRAM_START] = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x0B(void)
{
    uint8_t expected_result = 0x6D92;

    memory->rom[0x00] = 0x0B;
    memory->bc.value = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(expected_result, memory->bc.value);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x0C(void)
{
    uint8_t expected_result = 0x52;
    memory->rom[0x00] = 0x0C;
    memory->bc.low = expected_result - 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x0C_zero_flag(void)
{
    uint8_t expected_result = 0x00;
    memory->rom[0x00] = 0x0C;
    memory->bc.low = 0xFF;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x0C_half_carry_flag(void)
{
    uint8_t expected_result = 0x10;
    memory->rom[0x00] = 0x0C;
    memory->bc.low = 0x0F;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x0D(void)
{
    uint8_t expected_result = 0x52;
    memory->rom[0x00] = 0x0D;
    memory->bc.low = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x0D_zero_flag(void)
{
    uint8_t expected_result = 0x00;
    memory->rom[0x00] = 0x0D;
    memory->bc.low = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x0D_half_carry_flag(void)
{
    uint8_t expected_result = 0xFF;
    memory->rom[0x00] = 0x0D;
    memory->bc.low = 0x00;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x0E(void)
{
    uint8_t expected_result = 0xD3;

    memory->rom[0x00] = 0x0E;
    memory->rom[0x01] = expected_result;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0002, memory->program_counter);
}

void test_cpu_step_opcode_0x0F(void)
{
    uint8_t expected_result = 0x78;

    memory->rom[0x00] = 0x0F;
    memory->af.high = 0xF0;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x0F_flag_regression(void)
{
    uint8_t expected_result = 0x01;

    memory->rom[0x00] = 0x0F;
    memory->af.high = 0x02;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x0F_carry_flag(void)
{
    uint8_t expected_result = 0x80;

    memory->rom[0x00] = 0x0F;
    memory->af.high = 0x01;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x11(void)
{
    uint16_t expected_result = 0x732E;

    memory->rom[0x00] = 0x11;
    memory->rom[0x01] = 0x2E;
    memory->rom[0x02] = 0x73;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(expected_result, memory->de.value);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0003, memory->program_counter);
}

void test_cpu_step_opcode_0x12(void)
{
    uint16_t address = 0xC8A3;
    uint8_t expected_result = 0x43;

    memory->rom[0x00] = 0x12;
    memory->af.high = expected_result;
    memory->de.value = address;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->wram[address - WRAM_START]);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x13(void)
{
    uint16_t expected_result = 0x03E8;

    memory->rom[0x00] = 0x13;
    memory->de.value = expected_result - 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(expected_result, memory->de.value);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x14(void)
{
    uint8_t expected_result = 0x52;
    memory->rom[0x00] = 0x14;
    memory->de.high = expected_result - 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x14_zero_flag(void)
{
    uint8_t expected_result = 0x00;
    memory->rom[0x00] = 0x14;
    memory->de.high = 0xFF;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x14_half_carry_flag(void)
{
    uint8_t expected_result = 0x10;
    memory->rom[0x00] = 0x14;
    memory->de.high = 0x0F;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x15(void)
{
    uint8_t expected_result = 0x52;
    memory->rom[0x00] = 0x15;
    memory->de.high = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x15_zero_flag(void)
{
    uint8_t expected_result = 0x00;
    memory->rom[0x00] = 0x15;
    memory->de.high = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x15_half_carry_flag(void)
{
    uint8_t expected_result = 0xFF;
    memory->rom[0x00] = 0x15;
    memory->de.high = 0x00;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x16(void)
{
    uint8_t expected_result = 0xD3;

    memory->rom[0x00] = 0x16;
    memory->rom[0x01] = expected_result;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0002, memory->program_counter);
}

void test_cpu_step_opcode_0x17(void)
{
    uint8_t expected = 0x1F;
    memory->rom[0x00] = 0x17;
    memory->af.high = 0x0F;

    set_register_flag(memory, C, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x17_flag_regression(void)
{
    uint8_t expected = 0x01;
    memory->rom[0x00] = 0x17;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x17_carry_flag(void)
{
    uint8_t expected = 0x00;
    memory->rom[0x00] = 0x17;

    memory->af.high = 0x80;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->af.high);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x18(void)
{
    memory->rom[0x00] = 0x18;
    memory->rom[0x01] = 0x05;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(0x0007, memory->program_counter);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x18_negative(void)
{
    memory->program_counter = 0x0005;
    memory->rom[0x05] = 0x18;
    memory->rom[0x06] = 0xFB;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(0x0002, memory->program_counter);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));

}

void test_cpu_step_opcode_0x19(void)
{
    uint16_t expected_addition_result = 0xFB9F;

    memory->de.value = 0x3A7C;
    memory->hl.value = 0xC123;
    memory->rom[0x00] = 0x19;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(expected_addition_result, memory->hl.value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x19_half_carry(void)
{
    uint16_t expected_addition_result = 0xE09F;

    memory->de.value = 0x2A7C;
    memory->hl.value = 0xB623;
    memory->rom[0x00] = 0x19;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(expected_addition_result, memory->hl.value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x19_carry(void)
{
    uint16_t expected_addition_result = 0x0000;

    memory->de.value = 0xFFFF;
    memory->hl.value = 0x0001;
    memory->rom[0x00] = 0x19;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(expected_addition_result, memory->hl.value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x1A(void)
{
    uint16_t address = 0xC8D4;
    uint8_t expected_value = 0xA5;

    memory->rom[0x00] = 0x1A;
    memory->de.value = address;
    memory->wram[address - WRAM_START] = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x1B(void)
{
    uint8_t expected_result = 0x6D92;

    memory->rom[0x00] = 0x1B;
    memory->de.value = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(expected_result, memory->de.value);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x1C(void)
{
    uint8_t expected_result = 0x52;
    memory->rom[0x00] = 0x1C;
    memory->de.low = expected_result - 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x1C_zero_flag(void)
{
    uint8_t expected_result = 0x00;
    memory->rom[0x00] = 0x1C;
    memory->de.low = 0xFF;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x1C_half_carry_flag(void)
{
    uint8_t expected_result = 0x10;
    memory->rom[0x00] = 0x1C;
    memory->de.low = 0x0F;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x1D(void)
{
    uint8_t expected_result = 0x52;
    memory->rom[0x00] = 0x1D;
    memory->de.low = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x1D_zero_flag(void)
{
    uint8_t expected_result = 0x00;
    memory->rom[0x00] = 0x1D;
    memory->de.low = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x1D_half_carry_flag(void)
{
    uint8_t expected_result = 0xFF;
    memory->rom[0x00] = 0x1D;
    memory->de.low = 0x00;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x1E(void)
{
    uint8_t expected_result = 0xD3;

    memory->rom[0x00] = 0x1E;
    memory->rom[0x01] = expected_result;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0002, memory->program_counter);
}

void test_cpu_step_opcode_0x1F(void)
{
    uint8_t expected = 0xF8;
    memory->rom[0x00] = 0x1F;
    memory->af.high = 0xF0;

    set_register_flag(memory, C, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x1F_flag_regression(void)
{
    uint8_t expected = 0x80;
    memory->rom[0x00] = 0x1F;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x1F_carry_flag(void)
{
    uint8_t expected = 0x00;
    memory->rom[0x00] = 0x1F;

    memory->af.high = 0x01;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->af.high);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x20_z_flag_false(void)
{
    memory->rom[0x00] = 0x20;
    memory->rom[0x01] = 0x05;

    set_register_flag(memory, Z, false);

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));

    TEST_ASSERT_EQUAL_UINT16(0x0007, memory->program_counter);
}

void test_cpu_step_opcode_0x20_z_flag_true(void)
{
    memory->rom[0x00] = 0x20;

    set_register_flag(memory, Z, true);

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0002, memory->program_counter);
}

void test_cpu_step_opcode_0x21(void)
{
    uint8_t expected_low_value = 0x3A;
    uint8_t expected_high_value = 0xE;

    memory->rom[0x00] = 0x21;
    memory->rom[0x01] = expected_low_value;
    memory->rom[0x02] = expected_high_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(expected_low_value, memory->hl.low);
    TEST_ASSERT_EQUAL_UINT8(expected_high_value, memory->hl.high);
    TEST_ASSERT_EQUAL_UINT16(0x0003, memory->program_counter);
}

void test_cpu_step_opcode_0x22(void)
{
    uint8_t expected_a_value = 0x85;
    uint16_t expexted_incremented_address_value = 0xC001;
    uint16_t address = 0xC000;

    memory->rom[0x00] = 0x22;
    memory->hl.value = address;
    memory->af.high = expected_a_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(memory->wram[address - WRAM_START], expected_a_value);
    TEST_ASSERT_EQUAL_UINT16(memory->hl.value, expexted_incremented_address_value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x23(void)
{
    uint16_t value = 0x09A2;
    uint16_t expected_value = 0x09A3;

    memory->rom[0x00] = 0x23;
    memory->hl.value = value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(memory->hl.value, expected_value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x24(void)
{
    uint8_t expected_result = 0x52;
    memory->rom[0x00] = 0x24;
    memory->hl.high = expected_result - 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x24_zero_flag(void)
{
    uint8_t expected_result = 0x00;
    memory->rom[0x00] = 0x24;
    memory->hl.high = 0xFF;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x24_half_carry_flag(void)
{
    uint8_t expected_result = 0x10;
    memory->rom[0x00] = 0x24;
    memory->hl.high = 0x0F;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x25(void)
{
    uint8_t expected_result = 0x52;
    memory->rom[0x00] = 0x25;
    memory->hl.high = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x25_zero_flag(void)
{
    uint8_t expected_result = 0x00;
    memory->rom[0x00] = 0x25;
    memory->hl.high = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x25_half_carry_flag(void)
{
    uint8_t expected_result = 0xFF;
    memory->rom[0x00] = 0x25;
    memory->hl.high = 0x00;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x26(void)
{
    uint8_t expected_result = 0x97;

    memory->rom[0x00] = 0x26;
    memory->rom[0x01] = expected_result;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0002, memory->program_counter);
}

void test_cpu_step_opcode_0x27(void)
{
    uint8_t expected_result = 0x77;

    memory->rom[0x00] = 0x27;
    memory->af.high = expected_result;
   
    set_all_flags_false();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x27_add_ones(void)
{
    uint8_t incorrect_value = 0x6B;
    uint8_t expected_result = 0x71;

    memory->rom[0x00] = 0x27;
    memory->af.high = incorrect_value;

    set_register_flag(memory, H, true);
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, false);
    set_register_flag(memory, Z, false);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x27_add_tens(void)
{
    uint8_t incorrect_value = 0xA2;
    uint8_t expected_result = 0x02;

    memory->rom[0x00] = 0x27;
    memory->af.high = incorrect_value;

    set_register_flag(memory, H, false);
    set_register_flag(memory, C, true);
    set_register_flag(memory, N, false);
    set_register_flag(memory, Z, false);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x27_add_tens_zero_flag(void)
{
    uint8_t incorrect_value = 0xA0;
    uint8_t expected_result = 0x00;

    memory->rom[0x00] = 0x27;
    memory->af.high = incorrect_value;

    set_register_flag(memory, H, false);
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, false);
    set_register_flag(memory, Z, false);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}


void test_cpu_step_opcode_0x27_substract_ones(void)
{
    uint8_t incorrect_value = 0x0D;
    uint8_t expected_result = 0x07;

    memory->rom[0x00] = 0x27;
    memory->af.high = incorrect_value;

    set_register_flag(memory, H, true);
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, false);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x27_substract_tens(void)
{
    uint8_t incorrect_value = 0xE4;
    uint8_t expected_result = 0x84;

    memory->rom[0x00] = 0x27;
    memory->af.high = incorrect_value;

    set_register_flag(memory, H, false);
    set_register_flag(memory, C, true);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, false);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x28_zflag_true(void)
{
    memory->rom[0x00] = 0x28;
    memory->rom[0x01] = 0x05;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(0x0007, memory->program_counter);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x28_negative_zflag_true(void)
{
    memory->program_counter = 0x0005;
    memory->rom[0x05] = 0x28;
    memory->rom[0x06] = 0xFB;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(0x0002, memory->program_counter);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x28_zflag_false(void)
{
    memory->program_counter = 0x0005;
    memory->rom[0x05] = 0x28;
    memory->rom[0x06] = 0xFB;

    set_register_flag(memory, H, true);
    set_register_flag(memory, C, true);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, false);

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(0x0007, memory->program_counter);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x29(void)
{
    uint16_t expected_addition_result = 0x2222;

    memory->hl.value = 0x1111;
    memory->rom[0x00] = 0x29;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(expected_addition_result, memory->hl.value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x29_half_carry(void)
{
    uint16_t expected_addition_result = 0x1000;

    memory->hl.value = 0x0800;
    memory->rom[0x00] = 0x29;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(expected_addition_result, memory->hl.value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x29_carry(void)
{
    uint16_t expected_addition_result = 0x0000;

    memory->hl.value = 0x8000;
    memory->rom[0x00] = 0x29;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(expected_addition_result, memory->hl.value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x2A(void)
{
    uint16_t address = 0xC8D4;
    uint8_t expected_value = 0xA5;
    uint16_t expected_address = 0xC8D5;

    memory->rom[0x00] = 0x2A;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_EQUAL_UINT16(expected_address, memory->hl.value);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x2B(void)
{
    uint8_t expected_result = 0x6D92;

    memory->rom[0x00] = 0x2B;
    memory->hl.value = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(expected_result, memory->hl.value);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x2C(void)
{
    uint8_t expected_result = 0x52;
    memory->rom[0x00] = 0x2C;
    memory->hl.low = expected_result - 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->hl.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x2C_zero_flag(void)
{
    uint8_t expected_result = 0x00;
    memory->rom[0x00] = 0x2C;
    memory->hl.low = 0xFF;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->hl.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x2C_half_carry_flag(void)
{
    uint8_t expected_result = 0x10;
    memory->rom[0x00] = 0x2C;
    memory->hl.low = 0x0F;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->hl.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x2D(void)
{
    uint8_t expected_result = 0x52;
    memory->rom[0x00] = 0x2D;
    memory->hl.low = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->hl.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x2D_zero_flag(void)
{
    uint8_t expected_result = 0x00;
    memory->rom[0x00] = 0x2D;
    memory->hl.low = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->hl.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x2D_half_carry_flag(void)
{
    uint8_t expected_result = 0xFF;
    memory->rom[0x00] = 0x2D;
    memory->hl.low = 0x00;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->hl.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x2E(void)
{
    uint8_t expected_result = 0xD3;

    memory->rom[0x00] = 0x2E;
    memory->rom[0x01] = expected_result;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->hl.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0002, memory->program_counter);
}

void test_cpu_step_opcode_0x2F(void)
{
    uint8_t expected_result = 0x00;

    memory->rom[0x00] = 0x2F;
    memory->af.high = 0xFF;
    set_all_flags_false();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x30_c_flag_false(void)
{
    memory->rom[0x00] = 0x30;
    memory->rom[0x01] = 0x05;

    set_register_flag(memory, C, false);

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));

    TEST_ASSERT_EQUAL_UINT16(0x0007, memory->program_counter);
}

void test_cpu_step_opcode_0x30_c_flag_true(void)
{
    memory->rom[0x00] = 0x30;

    set_register_flag(memory, C, true);

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0002, memory->program_counter);
}

void test_cpu_step_opcode_0x31(void)
{
    uint8_t expected_low_value = 0x3A;
    uint8_t expected_high_value = 0xE0;

    uint16_t expected_value = 0xE03A;

    memory->rom[0x00] = 0x31;
    memory->rom[0x01] = expected_low_value;
    memory->rom[0x02] = expected_high_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(expected_value, memory->stack_pointer);
    TEST_ASSERT_EQUAL_UINT16(0x0003, memory->program_counter);
}

void test_cpu_step_opcode_0x32(void)
{
    uint8_t expected_a_value = 0x85;
    uint16_t expexted_decremented_address_value = 0xBFFF;
    uint16_t address = 0xC000;

    memory->rom[0x00] = 0x32;
    memory->hl.value = address;
    memory->af.high = expected_a_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(memory->wram[address - WRAM_START], expected_a_value);
    TEST_ASSERT_EQUAL_UINT16(memory->hl.value, expexted_decremented_address_value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x33(void)
{
    uint16_t value = 0x09A2;
    uint16_t expected_value = 0x09A3;

    memory->rom[0x00] = 0x33;
    memory->stack_pointer = value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(memory->stack_pointer, expected_value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x34(void)
{
    uint8_t value = 0x77;
    uint8_t expected_value = 0x78;
    uint16_t address = 0xC0A7;

    memory->rom[0x00] = 0x34;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(memory->wram[address - WRAM_START], expected_value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x34_h_flag(void)
{
    uint8_t value = 0x0F;
    uint8_t expected_value = 0x10;
    uint16_t address = 0xC0A7;

    memory->rom[0x00] = 0x34;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(memory->wram[address - WRAM_START], expected_value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x34_z_flag(void)
{
    uint8_t value = 0xFF;
    uint8_t expected_value = 0x00;
    uint16_t address = 0xC0A7;

    memory->rom[0x00] = 0x34;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(memory->wram[address - WRAM_START], expected_value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x35(void)
{
    uint8_t value = 0x77;
    uint8_t expected_value = 0x76;
    uint16_t address = 0xC0A7;

    memory->rom[0x00] = 0x35;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(memory->wram[address - WRAM_START], expected_value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x35_h_flag(void)
{
    uint8_t value = 0x00;
    uint8_t expected_value = 0xFF;
    uint16_t address = 0xC0A7;

    memory->rom[0x00] = 0x35;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(memory->wram[address - WRAM_START], expected_value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x35_z_flag(void)
{
    uint8_t value = 0x01;
    uint8_t expected_value = 0x00;
    uint16_t address = 0xC0A7;

    memory->rom[0x00] = 0x35;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(memory->wram[address - WRAM_START], expected_value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x36(void)
{
    uint8_t value = 0xA7;
    uint16_t address = 0xC46E;

    memory->rom[0x00] = 0x36;
    memory->rom[0x01] = value;
    memory->hl.value = address;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(memory->wram[address - WRAM_START], value);
    TEST_ASSERT_EQUAL_UINT16(0x0002, memory->program_counter);
}

void test_cpu_step_opcode_0x37_all_false(void)
{
    memory->rom[0x00] = 0x37;
    set_all_flags_false();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x37_all_true(void)
{
    memory->rom[0x00] = 0x37;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x38_cflag_true(void)
{
    memory->rom[0x00] = 0x28;
    memory->rom[0x01] = 0x05;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(0x0007, memory->program_counter);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x38_negative_cflag_true(void)
{
    memory->program_counter = 0x0005;
    memory->rom[0x05] = 0x38;
    memory->rom[0x06] = 0xFB;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(0x0002, memory->program_counter);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x38_cflag_false(void)
{
    memory->program_counter = 0x0005;
    memory->rom[0x05] = 0x38;
    memory->rom[0x06] = 0xFB;

    set_register_flag(memory, H, true);
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(0x0007, memory->program_counter);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x39(void)
{
    uint16_t expected_addition_result = 0xFB9F;

    memory->hl.value = 0x3A7C;
    memory->stack_pointer = 0xC123;
    memory->rom[0x00] = 0x39;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(expected_addition_result, memory->hl.value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x39_half_carry(void)
{
    uint16_t expected_addition_result = 0xE09F;

    memory->hl.value = 0x2A7C;
    memory->stack_pointer = 0xB623;
    memory->rom[0x00] = 0x39;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(expected_addition_result, memory->hl.value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x39_carry(void)
{
    uint16_t expected_addition_result = 0x0000;

    memory->hl.value = 0xFFFF;
    memory->stack_pointer = 0x0001;
    memory->rom[0x00] = 0x39;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(expected_addition_result, memory->hl.value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x3A(void)
{
    uint16_t address = 0xC8D4;
    uint8_t expected_value = 0xA5;
    uint16_t expected_address = 0xC8D3;

    memory->rom[0x00] = 0x3A;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_EQUAL_UINT16(expected_address, memory->hl.value);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x3B(void)
{
    uint8_t expected_result = 0x6D92;

    memory->rom[0x00] = 0x3B;
    memory->stack_pointer = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(expected_result, memory->stack_pointer);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x3C(void)
{
    uint8_t expected_result = 0x52;
    memory->rom[0x00] = 0x3C;
    memory->af.high = expected_result - 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x3C_zero_flag(void)
{
    uint8_t expected_result = 0x00;
    memory->rom[0x00] = 0x3C;
    memory->af.high = 0xFF;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x3C_half_carry_flag(void)
{
    uint8_t expected_result = 0x10;
    memory->rom[0x00] = 0x3C;
    memory->af.high = 0x0F;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x3D(void)
{
    uint8_t expected_result = 0x52;
    memory->rom[0x00] = 0x3D;
    memory->af.high = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x3D_zero_flag(void)
{
    uint8_t expected_result = 0x00;
    memory->rom[0x00] = 0x3D;
    memory->af.high = expected_result + 1;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x3D_half_carry_flag(void)
{
    uint8_t expected_result = 0xFF;
    memory->rom[0x00] = 0x3D;
    memory->af.high = 0x00;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x3E(void)
{
    uint8_t expected_result = 0xD3;

    memory->rom[0x00] = 0x3E;
    memory->rom[0x01] = expected_result;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_result, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0002, memory->program_counter);
}

void test_cpu_step_opcode_0x3F_true(void)
{
    memory->rom[0x00] = 0x3F;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x3F_false(void)
{
    memory->rom[0x00] = 0x3F;
    set_all_flags_false();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x40(void)
{
    uint8_t expected_value = 0xE3;

    memory->rom[0x00] = 0x40;
    memory->bc.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x41(void)
{
    uint8_t expected_value = 0x46;

    memory->rom[0x00] = 0x41;
    memory->bc.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x42(void)
{
    uint8_t expected_value = 0x0A;

    memory->rom[0x00] = 0x42;
    memory->de.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x43(void)
{
    uint8_t expected_value = 0x26;

    memory->rom[0x00] = 0x43;
    memory->de.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x44(void)
{
    uint8_t expected_value = 0x89;

    memory->rom[0x00] = 0x44;
    memory->hl.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x45(void)
{
    uint8_t expected_value = 0x5E;

    memory->rom[0x00] = 0x45;
    memory->hl.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x46(void)
{
    uint16_t address = 0xC7A3l;
    uint8_t expected_value = 0x5E;

    memory->rom[0x00] = 0x46;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x47(void)
{
    uint8_t expected_value = 0x94;

    memory->rom[0x00] = 0x47;
    memory->af.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x48(void)
{
    uint8_t expected_value = 0xB3;

    memory->rom[0x00] = 0x48;
    memory->bc.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x49(void)
{
    uint8_t expected_value = 0x84;

    memory->rom[0x00] = 0x49;
    memory->bc.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x4A(void)
{
    uint8_t expected_value = 0xBC;

    memory->rom[0x00] = 0x4A;
    memory->de.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x4B(void)
{
    uint8_t expected_value = 0x76;

    memory->rom[0x00] = 0x4B;
    memory->de.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x4C(void)
{
    uint8_t expected_value = 0x78;

    memory->rom[0x00] = 0x4C;
    memory->hl.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x4D(void)
{
    uint8_t expected_value = 0x63;

    memory->rom[0x00] = 0x4D;
    memory->hl.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x4E(void)
{
    uint16_t address = 0xCE5D;
    uint8_t expected_value = 0x70;

    memory->rom[0x00] = 0x4E;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x4F(void)
{
    uint8_t expected_value = 0xD4;

    memory->rom[0x00] = 0x4F;
    memory->af.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->bc.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x50(void)
{
    uint8_t expected_value = 0x47;

    memory->rom[0x00] = 0x50;
    memory->bc.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x51(void)
{
    uint8_t expected_value = 0xA0;

    memory->rom[0x00] = 0x51;
    memory->bc.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x52(void)
{
    uint8_t expected_value = 0x6F;

    memory->rom[0x00] = 0x52;
    memory->de.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x53(void)
{
    uint8_t expected_value = 0xB4;

    memory->rom[0x00] = 0x53;
    memory->de.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x54(void)
{
    uint8_t expected_value = 0x6C;

    memory->rom[0x00] = 0x54;
    memory->hl.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x55(void)
{
    uint8_t expected_value = 0x87;

    memory->rom[0x00] = 0x55;
    memory->hl.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x56(void)
{
    uint16_t address = 0xC7A4;
    uint8_t expected_value = 0xAA;

    memory->rom[0x00] = 0x56;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x57(void)
{
    uint8_t expected_value = 0x26;

    memory->rom[0x00] = 0x57;
    memory->af.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x58(void)
{
    uint8_t expected_value = 0x71;

    memory->rom[0x00] = 0x58;
    memory->bc.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x59(void)
{
    uint8_t expected_value = 0xBE;

    memory->rom[0x00] = 0x59;
    memory->bc.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x5A(void)
{
    uint8_t expected_value = 0xDE;

    memory->rom[0x00] = 0x5A;
    memory->de.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x5B(void)
{
    uint8_t expected_value = 0xB9;

    memory->rom[0x00] = 0x5B;
    memory->de.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x5C(void)
{
    uint8_t expected_value = 0xB8;

    memory->rom[0x00] = 0x5C;
    memory->hl.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x5D(void)
{
    uint8_t expected_value = 0x51;

    memory->rom[0x00] = 0x5D;
    memory->hl.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x5E(void)
{
    uint16_t address = 0xC8D6;
    uint8_t expected_value = 0x85;

    memory->rom[0x00] = 0x5E;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x5F(void)
{
    uint8_t expected_value = 0xBA;

    memory->rom[0x00] = 0x5F;
    memory->af.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->de.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x60(void)
{
    uint8_t expected_value = 0xF2;

    memory->rom[0x00] = 0x60;
    memory->bc.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x61(void)
{
    uint8_t expected_value = 0x39;

    memory->rom[0x00] = 0x61;
    memory->bc.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x62(void)
{
    uint8_t expected_value = 0x33;

    memory->rom[0x00] = 0x62;
    memory->de.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x63(void)
{
    uint8_t expected_value = 0x18;

    memory->rom[0x00] = 0x63;
    memory->de.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x64(void)
{
    uint8_t expected_value = 0xC3;

    memory->rom[0x00] = 0x64;
    memory->hl.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x65(void)
{
    uint8_t expected_value = 0x53;

    memory->rom[0x00] = 0x65;
    memory->hl.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x66(void)
{
    uint16_t address = 0xCA7B;
    uint8_t expected_value = 0xE7;

    memory->rom[0x00] = 0x66;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x67(void)
{
    uint8_t expected_value = 0xE9;

    memory->rom[0x00] = 0x67;
    memory->af.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x68(void)
{
    uint8_t expected_value = 0xCB;

    memory->rom[0x00] = 0x68;
    memory->bc.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x69(void)
{
    uint8_t expected_value = 0xB6;

    memory->rom[0x00] = 0x69;
    memory->bc.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x6A(void)
{
    uint8_t expected_value = 0xD3;

    memory->rom[0x00] = 0x6A;
    memory->de.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x6B(void)
{
    uint8_t expected_value = 0xF7;

    memory->rom[0x00] = 0x6B;
    memory->de.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x6C(void)
{
    uint8_t expected_value = 0x5E;

    memory->rom[0x00] = 0x6C;
    memory->hl.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x6D(void)
{
    uint8_t expected_value = 0x29;

    memory->rom[0x00] = 0x6D;
    memory->hl.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x6E(void)
{
    uint16_t address = 0xC4E8;
    uint8_t expected_value = 0x85;

    memory->rom[0x00] = 0x6E;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x6F(void)
{
    uint8_t expected_value = 0x0C;

    memory->rom[0x00] = 0x6F;
    memory->af.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->hl.low);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x70(void)
{
    uint16_t address = 0xCE31;
    uint8_t expected_value = 0x7E;

    memory->rom[0x00] = 0x70;
    memory->hl.value = address;
    memory->bc.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->wram[address - WRAM_START]);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x71(void)
{
    uint16_t address = 0xC91A;
    uint8_t expected_value = 0xE0;

    memory->rom[0x00] = 0x71;
    memory->hl.value = address;
    memory->bc.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->wram[address - WRAM_START]);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x72(void)
{
    uint16_t address = 0xCB72;
    uint8_t expected_value = 0xD4;

    memory->rom[0x00] = 0x72;
    memory->hl.value = address;
    memory->de.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->wram[address - WRAM_START]);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x73(void)
{
    uint16_t address = 0xC6B4;
    uint8_t expected_value = 0x68;

    memory->rom[0x00] = 0x73;
    memory->hl.value = address;
    memory->de.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->wram[address - WRAM_START]);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x74(void)
{
    uint16_t address = 0xCF28;
    uint8_t expected_value = 0xCF;

    memory->rom[0x00] = 0x74;
    memory->hl.value = address;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->wram[address - WRAM_START]);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x75(void)
{
    uint16_t address = 0xC3D9;
    uint8_t expected_value = 0xD9;

    memory->rom[0x00] = 0x75;
    memory->hl.value = address;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->wram[address - WRAM_START]);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x77(void)
{
    uint16_t address = 0xCC4E;
    uint8_t expected_value = 0x18;

    memory->rom[0x00] = 0x77;
    memory->hl.value = address;
    memory->af.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->wram[address - WRAM_START]);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x78(void)
{
    uint8_t expected_value = 0xF0;

    memory->rom[0x00] = 0x78;
    memory->bc.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x79(void)
{
    uint8_t expected_value = 0x7B;

    memory->rom[0x00] = 0x79;
    memory->bc.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x7A(void)
{
    uint8_t expected_value = 0xAE;

    memory->rom[0x00] = 0x7A;
    memory->de.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x7B(void)
{
    uint8_t expected_value = 0x08;

    memory->rom[0x00] = 0x7B;
    memory->de.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x7C(void)
{
    uint8_t expected_value = 0xF4;

    memory->rom[0x00] = 0x7C;
    memory->hl.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x7D(void)
{
    uint8_t expected_value = 0xEB;

    memory->rom[0x00] = 0x7D;
    memory->hl.low = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x7E(void)
{
    uint16_t address = 0xC8F1;
    uint8_t expected_value = 0x1A;

    memory->rom[0x00] = 0x7E;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x7F(void)
{
    uint8_t expected_value = 0xBE;

    memory->rom[0x00] = 0x7F;
    memory->af.high = expected_value;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_TRUE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x80(void)
{
    uint8_t expected_value = 0x33;

    memory->rom[0x00] = 0x80;
    memory->af.high = 0x12;
    memory->bc.high = 0x21;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x80_zero_flag(void)
{
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x80;
    memory->af.high = 0x80;
    memory->bc.high = 0x80;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x80_carry_flag(void)
{
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x80;
    memory->af.high = 0x90;
    memory->bc.high = 0x90;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x80_half_flag(void)
{
    uint8_t expected_value = 0x10;

    memory->rom[0x00] = 0x80;
    memory->af.high = 0x09;
    memory->bc.high = 0x07;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x81(void)
{
    uint8_t expected_value = 0x33;

    memory->rom[0x00] = 0x81;
    memory->af.high = 0x12;
    memory->bc.low = 0x21;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x81_zero_flag(void)
{
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x81;
    memory->af.high = 0x80;
    memory->bc.low = 0x80;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x81_carry_flag(void)
{
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x81;
    memory->af.high = 0x90;
    memory->bc.low = 0x90;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x81_half_flag(void)
{
    uint8_t expected_value = 0x10;

    memory->rom[0x00] = 0x81;
    memory->af.high = 0x09;
    memory->bc.low = 0x07;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x82(void)
{
    uint8_t expected_value = 0x33;

    memory->rom[0x00] = 0x82;
    memory->af.high = 0x12;
    memory->de.high = 0x21;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x82_zero_flag(void)
{
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x82;
    memory->af.high = 0x80;
    memory->de.high = 0x80;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x82_carry_flag(void)
{
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x82;
    memory->af.high = 0x90;
    memory->de.high = 0x90;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x82_half_flag(void)
{
    uint8_t expected_value = 0x10;

    memory->rom[0x00] = 0x82;
    memory->af.high = 0x09;
    memory->de.high = 0x07;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x83(void)
{
    uint8_t expected_value = 0x33;

    memory->rom[0x00] = 0x83;
    memory->af.high = 0x12;
    memory->de.low = 0x21;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x83_zero_flag(void)
{
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x83;
    memory->af.high = 0x80;
    memory->de.low = 0x80;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x83_carry_flag(void)
{
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x83;
    memory->af.high = 0x90;
    memory->de.low = 0x90;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x83_half_flag(void)
{
    uint8_t expected_value = 0x10;

    memory->rom[0x00] = 0x83;
    memory->af.high = 0x09;
    memory->de.low = 0x07;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x84(void)
{
    uint8_t expected_value = 0x33;

    memory->rom[0x00] = 0x84;
    memory->af.high = 0x12;
    memory->hl.high = 0x21;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x84_zero_flag(void)
{
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x84;
    memory->af.high = 0x80;
    memory->hl.high = 0x80;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x84_carry_flag(void)
{
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x84;
    memory->af.high = 0x90;
    memory->hl.high = 0x90;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x84_half_flag(void)
{
    uint8_t expected_value = 0x10;

    memory->rom[0x00] = 0x84;
    memory->af.high = 0x09;
    memory->hl.high = 0x07;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x85(void)
{
    uint8_t expected_value = 0x33;

    memory->rom[0x00] = 0x85;
    memory->af.high = 0x12;
    memory->hl.low = 0x21;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x85_zero_flag(void)
{
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x85;
    memory->af.high = 0x80;
    memory->hl.low = 0x80;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x85_carry_flag(void)
{
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x85;
    memory->af.high = 0x90;
    memory->hl.low = 0x90;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x85_half_flag(void)
{
    uint8_t expected_value = 0x10;

    memory->rom[0x00] = 0x85;
    memory->af.high = 0x09;
    memory->hl.low = 0x07;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x86(void)
{
    uint16_t address = 0xC7A3;
    uint8_t expected_value = 0x33;

    memory->rom[0x00] = 0x86;
    memory->af.high = 0x12;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = 0x21;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x86_zero_flag(void)
{
    uint16_t address = 0xC8A7;
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x86;
    memory->af.high = 0x80;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = 0x80;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x86_carry_flag(void)
{
    uint16_t address = 0xCF3B;
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x86;
    memory->af.high = 0x90;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = 0x90;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x86_half_flag(void)
{
    uint16_t address = 0xC2D9;
    uint8_t expected_value = 0x10;

    memory->rom[0x00] = 0x86;
    memory->af.high = 0x09;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = 0x07;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x87(void)
{
    uint8_t expected_value = 0x24;

    memory->rom[0x00] = 0x87;
    memory->af.high = 0x12;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x87_zero_flag(void)
{
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x87;
    memory->af.high = 0x80;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x87_carry_flag(void)
{
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x87;
    memory->af.high = 0x90;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x87_half_flag(void)
{
    uint8_t expected_value = 0x12;

    memory->rom[0x00] = 0x87;
    memory->af.high = 0x09;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x88(void)
{
    uint8_t expected_value = 0x34;

    memory->rom[0x00] = 0x88;
    memory->af.high = 0x12;
    memory->bc.high = 0x21;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x88_zero_flag(void)
{
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x88;
    memory->af.high = 0x80;
    memory->bc.high = 0x80;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x88_carry_flag(void)
{
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x88;
    memory->af.high = 0x90;
    memory->bc.high = 0x90;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x88_half_flag(void)
{
    uint8_t expected_value = 0x10;

    memory->rom[0x00] = 0x88;
    memory->af.high = 0x09;
    memory->bc.high = 0x07;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x89(void)
{
    uint8_t expected_value = 0x34;

    memory->rom[0x00] = 0x89;
    memory->af.high = 0x12;
    memory->bc.low = 0x21;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x89_zero_flag(void)
{
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x89;
    memory->af.high = 0x80;
    memory->bc.low = 0x80;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x89_carry_flag(void)
{
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x89;
    memory->af.high = 0x90;
    memory->bc.low = 0x90;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x89_half_flag(void)
{
    uint8_t expected_value = 0x10;

    memory->rom[0x00] = 0x89;
    memory->af.high = 0x09;
    memory->bc.low = 0x07;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8A(void)
{
    uint8_t expected_value = 0x34;

    memory->rom[0x00] = 0x8A;
    memory->af.high = 0x12;
    memory->de.high = 0x21;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8A_zero_flag(void)
{
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x8A;
    memory->af.high = 0x80;
    memory->de.high = 0x80;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8A_carry_flag(void)
{
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x8A;
    memory->af.high = 0x90;
    memory->de.high = 0x90;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8A_half_flag(void)
{
    uint8_t expected_value = 0x10;

    memory->rom[0x00] = 0x8A;
    memory->af.high = 0x09;
    memory->de.high = 0x07;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8B(void)
{
    uint8_t expected_value = 0x34;

    memory->rom[0x00] = 0x8B;
    memory->af.high = 0x12;
    memory->de.low = 0x21;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8B_zero_flag(void)
{
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x8B;
    memory->af.high = 0x80;
    memory->de.low = 0x80;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8B_carry_flag(void)
{
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x8B;
    memory->af.high = 0x90;
    memory->de.low = 0x90;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8B_half_flag(void)
{
    uint8_t expected_value = 0x10;

    memory->rom[0x00] = 0x8B;
    memory->af.high = 0x09;
    memory->de.low = 0x07;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8C(void)
{
    uint8_t expected_value = 0x34;

    memory->rom[0x00] = 0x8C;
    memory->af.high = 0x12;
    memory->hl.high = 0x21;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8C_zero_flag(void)
{
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x8C;
    memory->af.high = 0x80;
    memory->hl.high = 0x80;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8C_carry_flag(void)
{
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x8C;
    memory->af.high = 0x90;
    memory->hl.high = 0x90;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8C_half_flag(void)
{
    uint8_t expected_value = 0x10;

    memory->rom[0x00] = 0x8C;
    memory->af.high = 0x09;
    memory->hl.high = 0x07;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8D(void)
{
    uint8_t expected_value = 0x34;

    memory->rom[0x00] = 0x8D;
    memory->af.high = 0x12;
    memory->hl.low = 0x21;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8D_zero_flag(void)
{
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x8D;
    memory->af.high = 0x80;
    memory->hl.low = 0x80;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8D_carry_flag(void)
{
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x8D;
    memory->af.high = 0x90;
    memory->hl.low = 0x90;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8D_half_flag(void)
{
    uint8_t expected_value = 0x10;

    memory->rom[0x00] = 0x8D;
    memory->af.high = 0x09;
    memory->hl.low = 0x07;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8E(void)
{
    uint16_t address = 0xCE71;
    uint8_t expected_value = 0x34;

    memory->rom[0x00] = 0x8E;
    memory->af.high = 0x12;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = 0x21;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8E_zero_flag(void)
{
    uint16_t address = 0xCE71;
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x8E;
    memory->af.high = 0x80;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = 0x80;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8E_carry_flag(void)
{
    uint16_t address = 0xCE71;
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x8E;
    memory->af.high = 0x90;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = 0x90;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8E_half_flag(void)
{
    uint16_t address = 0xCE71;
    uint8_t expected_value = 0x10;

    memory->rom[0x00] = 0x8E;
    memory->af.high = 0x09;
    memory->hl.value = address;
    memory->wram[address - WRAM_START] = 0x07;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8F(void)
{
    uint8_t expected_value = 0x25;

    memory->rom[0x00] = 0x87;
    memory->af.high = 0x12;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8F_zero_flag(void)
{
    uint8_t expected_value = 0x00;

    memory->rom[0x00] = 0x8F;
    memory->af.high = 0x80;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_TRUE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8F_carry_flag(void)
{
    uint8_t expected_value = 0x20;

    memory->rom[0x00] = 0x8F;
    memory->af.high = 0x90;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
}

void test_cpu_step_opcode_0x8F_half_flag(void)
{
    uint8_t expected_value = 0x12;

    memory->rom[0x00] = 0x8F;
    memory->af.high = 0x09;
    set_register_flag(memory, C, false);
    set_register_flag(memory, N, true);
    set_register_flag(memory, Z, true);
    set_register_flag(memory, H, true);

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected_value, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_TRUE(get_register_flag(memory, H));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_cpu_step_opcode_0x01);
    RUN_TEST(test_cpu_step_opcode_0x02);
    RUN_TEST(test_cpu_step_opcode_0x03);
    RUN_TEST(test_cpu_step_opcode_0x04);
    RUN_TEST(test_cpu_step_opcode_0x04_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x04_zero_flag_regression);
    RUN_TEST(test_cpu_step_opcode_0x04_half_carry);
    RUN_TEST(test_cpu_step_opcode_0x05);
    RUN_TEST(test_cpu_step_opcode_0x05_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x05_zero_flag_regression);
    RUN_TEST(test_cpu_step_opcode_0x05_half_carry);
    RUN_TEST(test_cpu_step_opcode_0x06);
    RUN_TEST(test_cpu_step_opcode_0x07);
    RUN_TEST(test_cpu_step_opcode_0x07_flag_regression);
    RUN_TEST(test_cpu_step_opcode_0x07_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x08);
    RUN_TEST(test_cpu_step_opcode_0x09);
    RUN_TEST(test_cpu_step_opcode_0x09_half_carry);
    RUN_TEST(test_cpu_step_opcode_0x09_carry);
    RUN_TEST(test_cpu_step_opcode_0x0A);
    RUN_TEST(test_cpu_step_opcode_0x0B);
    RUN_TEST(test_cpu_step_opcode_0x0C);
    RUN_TEST(test_cpu_step_opcode_0x0C_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x0C_half_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x0D);
    RUN_TEST(test_cpu_step_opcode_0x0D_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x0D_half_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x0E);
    RUN_TEST(test_cpu_step_opcode_0x0F);
    RUN_TEST(test_cpu_step_opcode_0x0F_flag_regression);
    RUN_TEST(test_cpu_step_opcode_0x0F_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x11);
    RUN_TEST(test_cpu_step_opcode_0x11);
    RUN_TEST(test_cpu_step_opcode_0x12);
    RUN_TEST(test_cpu_step_opcode_0x13);
    RUN_TEST(test_cpu_step_opcode_0x14);
    RUN_TEST(test_cpu_step_opcode_0x14_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x14_half_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x15);
    RUN_TEST(test_cpu_step_opcode_0x15_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x15_half_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x16);
    RUN_TEST(test_cpu_step_opcode_0x17);
    RUN_TEST(test_cpu_step_opcode_0x17_flag_regression);
    RUN_TEST(test_cpu_step_opcode_0x17_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x18);
    RUN_TEST(test_cpu_step_opcode_0x18_negative);
    RUN_TEST(test_cpu_step_opcode_0x19);
    RUN_TEST(test_cpu_step_opcode_0x19_half_carry);
    RUN_TEST(test_cpu_step_opcode_0x19_carry);
    RUN_TEST(test_cpu_step_opcode_0x1A);
    RUN_TEST(test_cpu_step_opcode_0x1B);
    RUN_TEST(test_cpu_step_opcode_0x1C);
    RUN_TEST(test_cpu_step_opcode_0x1C_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x1C_half_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x1D);
    RUN_TEST(test_cpu_step_opcode_0x1D_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x1D_half_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x1E);
    RUN_TEST(test_cpu_step_opcode_0x1F);
    RUN_TEST(test_cpu_step_opcode_0x1F_flag_regression);
    RUN_TEST(test_cpu_step_opcode_0x1F_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x20_z_flag_false);
    RUN_TEST(test_cpu_step_opcode_0x20_z_flag_true);
    RUN_TEST(test_cpu_step_opcode_0x21);
    RUN_TEST(test_cpu_step_opcode_0x22);
    RUN_TEST(test_cpu_step_opcode_0x23);
    RUN_TEST(test_cpu_step_opcode_0x24);
    RUN_TEST(test_cpu_step_opcode_0x24_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x24_half_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x25);
    RUN_TEST(test_cpu_step_opcode_0x25_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x25_half_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x26);
    RUN_TEST(test_cpu_step_opcode_0x27);
    RUN_TEST(test_cpu_step_opcode_0x27_add_ones);
    RUN_TEST(test_cpu_step_opcode_0x27_add_tens);
    RUN_TEST(test_cpu_step_opcode_0x27_add_tens_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x27_substract_ones);
    RUN_TEST(test_cpu_step_opcode_0x27_substract_tens);
    RUN_TEST(test_cpu_step_opcode_0x28_zflag_true);
    RUN_TEST(test_cpu_step_opcode_0x28_negative_zflag_true);
    RUN_TEST(test_cpu_step_opcode_0x28_zflag_false);
    RUN_TEST(test_cpu_step_opcode_0x29);
    RUN_TEST(test_cpu_step_opcode_0x29_half_carry);
    RUN_TEST(test_cpu_step_opcode_0x29_carry);
    RUN_TEST(test_cpu_step_opcode_0x2A);
    RUN_TEST(test_cpu_step_opcode_0x2B);
    RUN_TEST(test_cpu_step_opcode_0x2C);
    RUN_TEST(test_cpu_step_opcode_0x2C_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x2C_half_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x2D);
    RUN_TEST(test_cpu_step_opcode_0x2D_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x2D_half_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x2E);
    RUN_TEST(test_cpu_step_opcode_0x2F);
    RUN_TEST(test_cpu_step_opcode_0x30_c_flag_false);
    RUN_TEST(test_cpu_step_opcode_0x30_c_flag_true);
    RUN_TEST(test_cpu_step_opcode_0x31);
    RUN_TEST(test_cpu_step_opcode_0x32);
    RUN_TEST(test_cpu_step_opcode_0x33);
    RUN_TEST(test_cpu_step_opcode_0x34);
    RUN_TEST(test_cpu_step_opcode_0x34_h_flag);
    RUN_TEST(test_cpu_step_opcode_0x34_z_flag);
    RUN_TEST(test_cpu_step_opcode_0x35);
    RUN_TEST(test_cpu_step_opcode_0x35_h_flag);
    RUN_TEST(test_cpu_step_opcode_0x35_z_flag);
    RUN_TEST(test_cpu_step_opcode_0x36);
    RUN_TEST(test_cpu_step_opcode_0x37_all_false);
    RUN_TEST(test_cpu_step_opcode_0x37_all_true);
    RUN_TEST(test_cpu_step_opcode_0x38_cflag_true);
    RUN_TEST(test_cpu_step_opcode_0x38_negative_cflag_true);
    RUN_TEST(test_cpu_step_opcode_0x38_cflag_false);
    RUN_TEST(test_cpu_step_opcode_0x39);
    RUN_TEST(test_cpu_step_opcode_0x39_half_carry);
    RUN_TEST(test_cpu_step_opcode_0x39_carry);
    RUN_TEST(test_cpu_step_opcode_0x3A);
    RUN_TEST(test_cpu_step_opcode_0x3B);
    RUN_TEST(test_cpu_step_opcode_0x3C);
    RUN_TEST(test_cpu_step_opcode_0x3C_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x3C_half_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x3D);
    RUN_TEST(test_cpu_step_opcode_0x3D_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x3D_half_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x3E);
    RUN_TEST(test_cpu_step_opcode_0x3F_true);
    RUN_TEST(test_cpu_step_opcode_0x3F_false);
    RUN_TEST(test_cpu_step_opcode_0x40);
    RUN_TEST(test_cpu_step_opcode_0x41);
    RUN_TEST(test_cpu_step_opcode_0x42);
    RUN_TEST(test_cpu_step_opcode_0x43);
    RUN_TEST(test_cpu_step_opcode_0x44);
    RUN_TEST(test_cpu_step_opcode_0x45);
    RUN_TEST(test_cpu_step_opcode_0x46);
    RUN_TEST(test_cpu_step_opcode_0x47);
    RUN_TEST(test_cpu_step_opcode_0x48);
    RUN_TEST(test_cpu_step_opcode_0x49);
    RUN_TEST(test_cpu_step_opcode_0x4A);
    RUN_TEST(test_cpu_step_opcode_0x4B);
    RUN_TEST(test_cpu_step_opcode_0x4C);
    RUN_TEST(test_cpu_step_opcode_0x4D);
    RUN_TEST(test_cpu_step_opcode_0x4E);
    RUN_TEST(test_cpu_step_opcode_0x4F);
    RUN_TEST(test_cpu_step_opcode_0x50);
    RUN_TEST(test_cpu_step_opcode_0x51);
    RUN_TEST(test_cpu_step_opcode_0x52);
    RUN_TEST(test_cpu_step_opcode_0x53);
    RUN_TEST(test_cpu_step_opcode_0x54);
    RUN_TEST(test_cpu_step_opcode_0x55);
    RUN_TEST(test_cpu_step_opcode_0x56);
    RUN_TEST(test_cpu_step_opcode_0x57);
    RUN_TEST(test_cpu_step_opcode_0x58);
    RUN_TEST(test_cpu_step_opcode_0x59);
    RUN_TEST(test_cpu_step_opcode_0x5A);
    RUN_TEST(test_cpu_step_opcode_0x5B);
    RUN_TEST(test_cpu_step_opcode_0x5C);
    RUN_TEST(test_cpu_step_opcode_0x5D);
    RUN_TEST(test_cpu_step_opcode_0x5E);
    RUN_TEST(test_cpu_step_opcode_0x5F);
    RUN_TEST(test_cpu_step_opcode_0x60);
    RUN_TEST(test_cpu_step_opcode_0x61);
    RUN_TEST(test_cpu_step_opcode_0x62);
    RUN_TEST(test_cpu_step_opcode_0x63);
    RUN_TEST(test_cpu_step_opcode_0x64);
    RUN_TEST(test_cpu_step_opcode_0x65);
    RUN_TEST(test_cpu_step_opcode_0x66);
    RUN_TEST(test_cpu_step_opcode_0x67);
    RUN_TEST(test_cpu_step_opcode_0x68);
    RUN_TEST(test_cpu_step_opcode_0x69);
    RUN_TEST(test_cpu_step_opcode_0x6A);
    RUN_TEST(test_cpu_step_opcode_0x6B);
    RUN_TEST(test_cpu_step_opcode_0x6C);
    RUN_TEST(test_cpu_step_opcode_0x6D);
    RUN_TEST(test_cpu_step_opcode_0x6E);
    RUN_TEST(test_cpu_step_opcode_0x6F);
    RUN_TEST(test_cpu_step_opcode_0x70);
    RUN_TEST(test_cpu_step_opcode_0x71);
    RUN_TEST(test_cpu_step_opcode_0x72);
    RUN_TEST(test_cpu_step_opcode_0x73);
    RUN_TEST(test_cpu_step_opcode_0x74);
    RUN_TEST(test_cpu_step_opcode_0x75);
    RUN_TEST(test_cpu_step_opcode_0x77);
    RUN_TEST(test_cpu_step_opcode_0x78);
    RUN_TEST(test_cpu_step_opcode_0x79);
    RUN_TEST(test_cpu_step_opcode_0x7A);
    RUN_TEST(test_cpu_step_opcode_0x7B);
    RUN_TEST(test_cpu_step_opcode_0x7C);
    RUN_TEST(test_cpu_step_opcode_0x7D);
    RUN_TEST(test_cpu_step_opcode_0x7E);
    RUN_TEST(test_cpu_step_opcode_0x7F);
    RUN_TEST(test_cpu_step_opcode_0x80);
    RUN_TEST(test_cpu_step_opcode_0x80_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x80_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x80_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x81);
    RUN_TEST(test_cpu_step_opcode_0x81_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x81_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x81_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x82);
    RUN_TEST(test_cpu_step_opcode_0x82_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x82_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x82_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x83);
    RUN_TEST(test_cpu_step_opcode_0x83_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x83_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x83_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x84);
    RUN_TEST(test_cpu_step_opcode_0x84_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x84_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x84_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x85);
    RUN_TEST(test_cpu_step_opcode_0x85_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x85_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x85_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x86);
    RUN_TEST(test_cpu_step_opcode_0x86_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x86_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x86_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x87);
    RUN_TEST(test_cpu_step_opcode_0x87_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x87_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x87_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x88);
    RUN_TEST(test_cpu_step_opcode_0x88_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x88_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x88_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x89);
    RUN_TEST(test_cpu_step_opcode_0x89_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x89_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x89_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x8A);
    RUN_TEST(test_cpu_step_opcode_0x8A_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x8A_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x8A_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x8A_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x8B);
    RUN_TEST(test_cpu_step_opcode_0x8B_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x8B_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x8B_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x8B_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x8C);
    RUN_TEST(test_cpu_step_opcode_0x8C_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x8C_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x8C_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x8C_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x8D);
    RUN_TEST(test_cpu_step_opcode_0x8D_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x8D_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x8D_half_flag);
    RUN_TEST(test_cpu_step_opcode_0x8E);
    RUN_TEST(test_cpu_step_opcode_0x8E_zero_flag);
    RUN_TEST(test_cpu_step_opcode_0x8E_carry_flag);
    RUN_TEST(test_cpu_step_opcode_0x8E_half_flag);

    return UNITY_END();
}