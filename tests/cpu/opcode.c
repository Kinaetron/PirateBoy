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

    TEST_ASSERT_EQUAL_UINT8(12, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(low_byte, memory->bc.low);
    TEST_ASSERT_EQUAL_UINT8(high_byte, memory->bc.high);
    TEST_ASSERT_EQUAL_UINT16(0x0003, memory->program_counter);
}

void test_cpu_step_opcode_0x02(void)
{
    memory->af.high = 0x42;
    memory->af.low = 0xF0;
    memory->bc.value = 0xC050;

    memory->rom[0x00] = 0x02;

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(0x42, memory->wram[0xC050 - WRAM_START]);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x03(void)
{
    memory->bc.value = 0x0005;
    uint16_t result = 0x0006;

    memory->rom[0x00] = 0x03;

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT16(result, memory->bc.value);
    TEST_ASSERT_EQUAL_UINT16(0x0001, memory->program_counter);
}

void test_cpu_step_opcode_0x04(void)
{
    memory->bc.high = 0x05;
    memory->af.low = 0xF0;

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

void test_cpu_step_opcode_0x05()
{
    memory->bc.high = 0x05;
    memory->af.low = 0xF0;

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

    TEST_ASSERT_EQUAL_UINT8(8, cpu_step(memory));
    TEST_ASSERT_EQUAL_UINT8(high_byte, memory->bc.high);
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
    memory->af.low = 0xF0;

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
    return UNITY_END();
}