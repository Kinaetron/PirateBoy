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
    TEST_ASSERT_EQUAL_UINT16(0x0006, memory->program_counter);
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
    TEST_ASSERT_EQUAL_UINT16(0x0006, memory->program_counter);
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

    return UNITY_END();
}