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

static bool get_register_flag(CPU_Memory* memory, Flag flag) {
    return (memory->af.low >> flag) & 1;
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
    uint8_t expected = 0x78;

    memory->rom[0x00] = 0x0F;
    memory->af.high = 0xF0;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x0F_flag_regression(void)
{
    uint8_t expected = 0x01;
    memory->rom[0x00] = 0x0F;
    memory->af.high = 0x02;
    set_all_flags_true();

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->af.high);
    TEST_ASSERT_FALSE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x0F_carry_flag(void)
{
    uint8_t expected = 0x80;
    memory->rom[0x00] = 0x0F;

    memory->af.high = 0x01;

    TEST_ASSERT_EQUAL_UINT8(4, cpu_step(memory));
    TEST_ASSERT_TRUE(get_register_flag(memory, C));
    TEST_ASSERT_FALSE(get_register_flag(memory, N));
    TEST_ASSERT_FALSE(get_register_flag(memory, Z));
    TEST_ASSERT_FALSE(get_register_flag(memory, H));
    TEST_ASSERT_EQUAL_UINT8(expected, memory->af.high);
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
    return UNITY_END();
}