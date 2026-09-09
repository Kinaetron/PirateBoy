#include "cpu.h"
#include "memory.h"

#include <unity.h>
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CPU_Memory* memory = NULL;

void setUp(void) {
	memory = calloc(1, sizeof(CPU_Memory));
}

void tearDown(void)
{
	free(memory);
	memory = NULL;
}

static void raw_memory_write(CPU_Memory* memory, uint16_t address, uint8_t value)
{
	if (address >= ECHO_RAM_START && address <= ECHO_RAM_END) {
		memory->flat[address - (ECHO_RAM_START - WRAM_START)] = value;
		return;
	}

	memory->flat[address] = value;
}

static uint8_t json_u8(cJSON* obj, const char* key) {
	return (uint8_t)cJSON_GetObjectItem(obj, key)->valueint;
}

static uint16_t json_u16(cJSON* obj, const char* key) {
	return (uint16_t)cJSON_GetObjectItem(obj, key)->valueint;
}

static void apply_state(CPU_Memory* memory, cJSON* state)
{
	memory->af.high = json_u8(state, "a");
	memory->af.low	= json_u8(state, "f");
	memory->bc.high = json_u8(state, "b");
	memory->bc.low	= json_u8(state, "c");
	memory->de.high = json_u8(state, "d");
	memory->de.low  = json_u8(state, "e");
	memory->hl.high = json_u8(state, "h");
	memory->hl.low  = json_u8(state, "l");

	memory->program_counter.value = json_u16(state, "pc");
	memory->stack_pointer = json_u16(state, "sp");

	memory->flat[INTERRUPT_ENABLE_ADDR] = json_u8(state, "ie");

	memory->flat[INTERRUPT_FLAG_ADDR] = 0;

	cJSON* ram = cJSON_GetObjectItem(state, "ram");
	cJSON* entry;

	cJSON_ArrayForEach(entry, ram)
	{
		uint16_t address = (uint16_t)cJSON_GetArrayItem(entry, 0)->valueint;
		uint8_t value = (uint8_t)cJSON_GetArrayItem(entry, 1)->valueint;
		raw_memory_write(memory, address, value);
	}
}

static int state_matches(CPU_Memory* memory, cJSON* state, char* failure_detail, size_t detail_size)
{
	struct 
	{ 
		const char* name; 
		uint8_t actual; 
		uint8_t expected; 
	}

	byte_checks[] =
	{
		{ "a", memory->af.high, json_u8(state, "a") },
		{ "f", memory->af.low,  json_u8(state, "f") },
		{ "b", memory->bc.high, json_u8(state, "b") },
		{ "c", memory->bc.low,  json_u8(state, "c") },
		{ "d", memory->de.high, json_u8(state, "d") },
		{ "e", memory->de.low,  json_u8(state, "e") },
		{ "h", memory->hl.high, json_u8(state, "h") },
		{ "l", memory->hl.low,  json_u8(state, "l") },
	};

	for (size_t i = 0; i < sizeof(byte_checks) / sizeof(byte_checks[0]); i++)
	{
		if (byte_checks[i].actual != byte_checks[i].expected)
		{
			snprintf(failure_detail, detail_size, "register %s: expected 0x%02X, got 0x%02X",
				byte_checks[i].name, byte_checks[i].expected, byte_checks[i].actual);

			return 0;
		}
	}

	uint16_t expected_program_counter = json_u16(state, "pc");
	if (memory->program_counter.value != expected_program_counter)
	{
		snprintf(failure_detail, detail_size, "program counter: expected 0x%04X, got 0x%04X",
			expected_program_counter, memory->program_counter.value);

		return 0;
	}

	uint16_t expected_stack_pointer = json_u16(state, "sp");
	if (memory->stack_pointer != expected_stack_pointer)
	{
		snprintf(failure_detail, detail_size, "stack pointer: expected 0x%04X, got 0x%04X",
			expected_stack_pointer, memory->stack_pointer);

		return 0;
	}

	cJSON* ram = cJSON_GetObjectItem(state, "ram");
	cJSON* entry;
	cJSON_ArrayForEach(entry, ram)
	{
		uint16_t address = (uint16_t)cJSON_GetArrayItem(entry, 0)->valueint;
		uint8_t expected_value = (uint8_t)cJSON_GetArrayItem(entry, 1)->valueint;
		uint8_t actual_value = memory_read(memory, address);

		if (actual_value != expected_value)
		{
			snprintf(failure_detail, detail_size, "ram[0x%04X]: expected 0x%02X, got 0x%02X",
				address, expected_value, actual_value);
			return 0;
		}
	}

	return 1;
}

static void run_opcode_test_file(const char* filename)
{
	FILE* file = fopen(filename, "rb");
	TEST_ASSERT_NOT_NULL_MESSAGE(file, filename);

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* data = malloc(length + 1);
	fread(data, 1, length, file);
	data[length] = '\0';
	fclose(file);

	cJSON* root = cJSON_Parse(data);
	free(data);
	TEST_ASSERT_NOT_NULL_MESSAGE(root, "failed to parse JSON");

	int total = 0;
	int failed = 0;
	char first_failure[256] = { 0 };

	cJSON* test_case;
	cJSON_ArrayForEach(test_case, root)
	{
		total++;

		memset(memory->flat, 0, sizeof(memory->flat));

		memory->af.value = 0;
		memory->bc.value = 0;
		memory->de.value = 0;
		memory->hl.value = 0;

		memory->stack_pointer = 0;
		memory->program_counter.value = 0;

		cpu_reset_state();

		cJSON* initial = cJSON_GetObjectItem(test_case, "initial");
		apply_state(memory, initial);

		cpu_step(memory);

		cJSON* final_state = cJSON_GetObjectItem(test_case, "final");
		char detail[128];

		if (!state_matches(memory, final_state, detail, sizeof(detail)))
		{
			failed++;

			if (failed == 1)
			{
				const char* name = cJSON_GetObjectItem(test_case, "name")->valuestring;
				snprintf(first_failure, sizeof(first_failure), "[%s] %s", name, detail);
			}
		}
	}

	cJSON_Delete(root);

	char summary[320];
	snprintf(summary, sizeof(summary), "%d/%d cases passed. First failure: %s",
		total - failed, total, failed > 0 ? first_failure : "none");

	TEST_ASSERT_EQUAL_INT_MESSAGE(0, failed, summary);
}

void test_sm83_opcode_0x00(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/00.json");
}

void test_sm83_opcode_0x01(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/01.json");
}

void test_sm83_opcode_0x02(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/02.json");
}

void test_sm83_opcode_0x03(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/03.json");
}

void test_sm83_opcode_0x04(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/04.json");
}

void test_sm83_opcode_0x05(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/05.json");
}

void test_sm83_opcode_0x06(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/06.json");
}

void test_sm83_opcode_0x07(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/07.json");
}

void test_sm83_opcode_0x08(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/08.json");
}

void test_sm83_opcode_0x09(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/09.json");
}

void test_sm83_opcode_0x0A(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/0a.json");
}

void test_sm83_opcode_0x0B(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/0b.json");
}

void test_sm83_opcode_0x0C(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/0c.json");
}

void test_sm83_opcode_0x0D(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/0d.json");
}

void test_sm83_opcode_0x0E(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/0e.json");
}

void test_sm83_opcode_0x0F(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/0f.json");
}

void test_sm83_opcode_0x11(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/11.json");
}

void test_sm83_opcode_0x12(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/12.json");
}

void test_sm83_opcode_0x13(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/13.json");
}

void test_sm83_opcode_0x14(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/14.json");
}

void test_sm83_opcode_0x15(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/15.json");
}

void test_sm83_opcode_0x16(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/16.json");
}

void test_sm83_opcode_0x17(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/17.json");
}

void test_sm83_opcode_0x18(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/18.json");
}

void test_sm83_opcode_0x19(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/19.json");
}

void test_sm83_opcode_0x1A(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/1a.json");
}

void test_sm83_opcode_0x1B(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/1b.json");
}

void test_sm83_opcode_0x1C(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/1c.json");
}

void test_sm83_opcode_0x1D(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/1d.json");
}

void test_sm83_opcode_0x1E(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/1e.json");
}

void test_sm83_opcode_0x1F(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/1f.json");
}

void test_sm83_opcode_0x20(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/20.json");
}

void test_sm83_opcode_0x21(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/21.json");
}

void test_sm83_opcode_0x22(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/22.json");
}

void test_sm83_opcode_0x23(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/23.json");
}

void test_sm83_opcode_0x24(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/24.json");
}

void test_sm83_opcode_0x25(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/25.json");
}

void test_sm83_opcode_0x26(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/26.json");
}

void test_sm83_opcode_0x27(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/27.json");
}

void test_sm83_opcode_0x28(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/28.json");
}

void test_sm83_opcode_0x29(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/29.json");
}

void test_sm83_opcode_0x2A(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/2a.json");
}

void test_sm83_opcode_0x2B(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/2b.json");
}

void test_sm83_opcode_0x2C(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/2c.json");
}

void test_sm83_opcode_0x2D(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/2d.json");
}

void test_sm83_opcode_0x2E(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/2e.json");
}

void test_sm83_opcode_0x2F(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/2f.json");
}

void test_sm83_opcode_0x30(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/30.json");
}

void test_sm83_opcode_0x31(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/31.json");
}

void test_sm83_opcode_0x32(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/32.json");
}

void test_sm83_opcode_0x33(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/33.json");
}

void test_sm83_opcode_0x34(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/34.json");
}

void test_sm83_opcode_0x35(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/35.json");
}

void test_sm83_opcode_0x36(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/36.json");
}

void test_sm83_opcode_0x37(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/37.json");
}

void test_sm83_opcode_0x38(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/38.json");
}

void test_sm83_opcode_0x39(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/39.json");
}

void test_sm83_opcode_0x3A(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/3a.json");
}

void test_sm83_opcode_0x3B(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/3b.json");
}

void test_sm83_opcode_0x3C(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/3c.json");
}

void test_sm83_opcode_0x3D(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/3d.json");
}

void test_sm83_opcode_0x3E(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/3e.json");
}

void test_sm83_opcode_0x3F(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/3f.json");
}

void test_sm83_opcode_0x40(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/40.json");
}

void test_sm83_opcode_0x41(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/41.json");
}

void test_sm83_opcode_0x42(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/42.json");
}

void test_sm83_opcode_0x43(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/43.json");
}

void test_sm83_opcode_0x44(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/44.json");
}

void test_sm83_opcode_0x45(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/45.json");
}

void test_sm83_opcode_0x46(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/46.json");
}

void test_sm83_opcode_0x47(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/47.json");
}

void test_sm83_opcode_0x48(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/48.json");
}

void test_sm83_opcode_0x49(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/49.json");
}

void test_sm83_opcode_0x4A(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/4a.json");
}

void test_sm83_opcode_0x4B(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/4b.json");
}

void test_sm83_opcode_0x4C(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/4c.json");
}

void test_sm83_opcode_0x4D(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/4d.json");
}

void test_sm83_opcode_0x4E(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/4e.json");
}

void test_sm83_opcode_0x4F(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/4f.json");
}

void test_sm83_opcode_0x50(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/50.json");
}

void test_sm83_opcode_0x51(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/51.json");
}

void test_sm83_opcode_0x52(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/52.json");
}

void test_sm83_opcode_0x53(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/53.json");
}

void test_sm83_opcode_0x54(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/54.json");
}

void test_sm83_opcode_0x55(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/55.json");
}

void test_sm83_opcode_0x56(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/56.json");
}

void test_sm83_opcode_0x57(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/57.json");
}

void test_sm83_opcode_0x58(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/58.json");
}

void test_sm83_opcode_0x59(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/59.json");
}

void test_sm83_opcode_0x5A(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/5a.json");
}

void test_sm83_opcode_0x5B(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/5b.json");
}

void test_sm83_opcode_0x5C(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/5c.json");
}

void test_sm83_opcode_0x5D(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/5d.json");
}

void test_sm83_opcode_0x5E(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/5e.json");
}

void test_sm83_opcode_0x5F(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/5f.json");
}

void test_sm83_opcode_0x60(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/60.json");
}

void test_sm83_opcode_0x61(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/61.json");
}

void test_sm83_opcode_0x62(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/62.json");
}

void test_sm83_opcode_0x63(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/63.json");
}

void test_sm83_opcode_0x64(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/64.json");
}

void test_sm83_opcode_0x65(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/65.json");
}

void test_sm83_opcode_0x66(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/66.json");
}

void test_sm83_opcode_0x67(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/67.json");
}

void test_sm83_opcode_0x68(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/68.json");
}

void test_sm83_opcode_0x69(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/69.json");
}

void test_sm83_opcode_0x6A(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/6a.json");
}

void test_sm83_opcode_0x6B(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/6b.json");
}

void test_sm83_opcode_0x6C(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/6c.json");
}

void test_sm83_opcode_0x6D(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/6d.json");
}

void test_sm83_opcode_0x6E(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/6e.json");
}

void test_sm83_opcode_0x6F(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/6f.json");
}

void test_sm83_opcode_0x70(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/70.json");
}

void test_sm83_opcode_0x71(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/71.json");
}

void test_sm83_opcode_0x72(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/72.json");
}

void test_sm83_opcode_0x73(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/73.json");
}

void test_sm83_opcode_0x74(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/74.json");
}

void test_sm83_opcode_0x75(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/75.json");
}

void test_sm83_opcode_0x76(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/76.json");
}

void test_sm83_opcode_0x77(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/77.json");
}

void test_sm83_opcode_0x78(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/78.json");
}

void test_sm83_opcode_0x79(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/79.json");
}

void test_sm83_opcode_0x7A(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/7a.json");
}

void test_sm83_opcode_0x7B(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/7b.json");
}

void test_sm83_opcode_0x7C(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/7c.json");
}

void test_sm83_opcode_0x7D(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/7d.json");
}

void test_sm83_opcode_0x7E(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/7e.json");
}

void test_sm83_opcode_0x7F(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/7f.json");
}

void test_sm83_opcode_0x80(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/80.json");
}

void test_sm83_opcode_0x81(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/81.json");
}

void test_sm83_opcode_0x82(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/82.json");
}

void test_sm83_opcode_0x83(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/83.json");
}

void test_sm83_opcode_0x84(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/84.json");
}

void test_sm83_opcode_0x85(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/85.json");
}

void test_sm83_opcode_0x86(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/86.json");
}

void test_sm83_opcode_0x87(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/87.json");
}

void test_sm83_opcode_0x88(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/88.json");
}

void test_sm83_opcode_0x89(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/89.json");
}

void test_sm83_opcode_0x8A(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/8a.json");
}

void test_sm83_opcode_0x8B(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/8b.json");
}

void test_sm83_opcode_0x8C(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/8c.json");
}

void test_sm83_opcode_0x8D(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/8d.json");
}

void test_sm83_opcode_0x8E(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/8e.json");
}

void test_sm83_opcode_0x8F(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/8f.json");
}

void test_sm83_opcode_0x90(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/90.json");
}

void test_sm83_opcode_0x91(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/91.json");
}

void test_sm83_opcode_0x92(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/92.json");
}

void test_sm83_opcode_0x93(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/93.json");
}

void test_sm83_opcode_0x94(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/94.json");
}

void test_sm83_opcode_0x95(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/95.json");
}

void test_sm83_opcode_0x96(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/96.json");
}

void test_sm83_opcode_0x97(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/97.json");
}

void test_sm83_opcode_0x98(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/98.json");
}

void test_sm83_opcode_0x99(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/99.json");
}

void test_sm83_opcode_0x9A(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/9a.json");
}

void test_sm83_opcode_0x9B(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/9b.json");
}

void test_sm83_opcode_0x9C(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/9c.json");
}

void test_sm83_opcode_0x9D(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/9d.json");
}

void test_sm83_opcode_0x9E(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/9e.json");
}

void test_sm83_opcode_0x9F(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/9f.json");
}

void test_sm83_opcode_0xA0(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/a0.json");
}

void test_sm83_opcode_0xA1(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/a1.json");
}

void test_sm83_opcode_0xA2(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/a2.json");
}

void test_sm83_opcode_0xA3(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/a3.json");
}

void test_sm83_opcode_0xA4(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/a4.json");
}

void test_sm83_opcode_0xA5(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/a5.json");
}

void test_sm83_opcode_0xA6(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/a6.json");
}

void test_sm83_opcode_0xA7(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/a7.json");
}

void test_sm83_opcode_0xA8(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/a8.json");
}

void test_sm83_opcode_0xA9(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/a9.json");
}

void test_sm83_opcode_0xAA(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/aa.json");
}

void test_sm83_opcode_0xAB(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/ab.json");
}

void test_sm83_opcode_0xAC(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/ac.json");
}

void test_sm83_opcode_0xAD(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/ad.json");
}

void test_sm83_opcode_0xAE(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/ae.json");
}

void test_sm83_opcode_0xAF(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/af.json");
}

void test_sm83_opcode_0xB0(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/b0.json");
}

void test_sm83_opcode_0xB1(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/b1.json");
}

void test_sm83_opcode_0xB2(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/b2.json");
}

void test_sm83_opcode_0xB3(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/b3.json");
}

void test_sm83_opcode_0xB4(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/b4.json");
}

void test_sm83_opcode_0xB5(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/b5.json");
}

void test_sm83_opcode_0xB6(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/b6.json");
}

void test_sm83_opcode_0xB7(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/b7.json");
}

void test_sm83_opcode_0xB8(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/b8.json");
}

void test_sm83_opcode_0xB9(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/b9.json");
}

void test_sm83_opcode_0xBA(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/ba.json");
}

void test_sm83_opcode_0xBB(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/bb.json");
}

void test_sm83_opcode_0xBC(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/bc.json");
}

void test_sm83_opcode_0xBD(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/bd.json");
}

void test_sm83_opcode_0xBE(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/be.json");
}

void test_sm83_opcode_0xBF(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/bf.json");
}

void test_sm83_opcode_0xC0(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/c0.json");
}

void test_sm83_opcode_0xC1(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/c1.json");
}

void test_sm83_opcode_0xC2(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/c2.json");
}

void test_sm83_opcode_0xC3(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/c3.json");
}

void test_sm83_opcode_0xC4(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/c4.json");
}

void test_sm83_opcode_0xC5(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/c5.json");
}

void test_sm83_opcode_0xC6(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/c6.json");
}

void test_sm83_opcode_0xC7(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/c7.json");
}

void test_sm83_opcode_0xC8(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/c8.json");
}

void test_sm83_opcode_0xC9(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/c9.json");
}

void test_sm83_opcode_0xCA(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/ca.json");
}

void test_sm83_opcode_0xCC(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/cc.json");
}

void test_sm83_opcode_0xCD(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/cd.json");
}

void test_sm83_opcode_0xCE(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/ce.json");
}

void test_sm83_opcode_0xCF(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/cf.json");
}

void test_sm83_opcode_0xD0(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/d0.json");
}

void test_sm83_opcode_0xD1(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/d1.json");
}

void test_sm83_opcode_0xD2(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/d2.json");
}

void test_sm83_opcode_0xD5(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/d5.json");
}

void test_sm83_opcode_0xD6(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/d6.json");
}

void test_sm83_opcode_0xD7(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/d7.json");
}

void test_sm83_opcode_0xD8(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/d8.json");
}

void test_sm83_opcode_0xD9(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/d9.json");
}

void test_sm83_opcode_0xDA(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/da.json");
}

void test_sm83_opcode_0xDC(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/dc.json");
}

void test_sm83_opcode_0xDE(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/de.json");
}

void test_sm83_opcode_0xDF(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/df.json");
}

void test_sm83_opcode_0xE0(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/e0.json");
}

void test_sm83_opcode_0xE1(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/e1.json");
}

void test_sm83_opcode_0xE2(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/e2.json");
}

void test_sm83_opcode_0xE5(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/e5.json");
}

void test_sm83_opcode_0xE6(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/e6.json");
}

void test_sm83_opcode_0xE7(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/e7.json");
}

void test_sm83_opcode_0xE8(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/e8.json");
}

void test_sm83_opcode_0xE9(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/e9.json");
}

void test_sm83_opcode_0xEA(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/ea.json");
}

void test_sm83_opcode_0xEE(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/ee.json");
}

void test_sm83_opcode_0xEF(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/ef.json");
}

void test_sm83_opcode_0xF0(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/f0.json");
}

void test_sm83_opcode_0xF1(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/f1.json");
}

void test_sm83_opcode_0xF2(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/f2.json");
}

void test_sm83_opcode_0xF3(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/f3.json");
}

void test_sm83_opcode_0xF5(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/f5.json");
}

void test_sm83_opcode_0xF6(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/f6.json");
}

void test_sm83_opcode_0xF7(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/f7.json");
}

void test_sm83_opcode_0xF8(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/f8.json");
}

void test_sm83_opcode_0xF9(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/f9.json");
}

void test_sm83_opcode_0xFA(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/fa.json");
}

void test_sm83_opcode_0xFB(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/fb.json");
}

void test_sm83_opcode_0xFE(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/fe.json");
}

void test_sm83_opcode_0xFF(void) {
	run_opcode_test_file(SM83_TEST_DATA_DIR "/ff.json");
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_sm83_opcode_0x00);
	RUN_TEST(test_sm83_opcode_0x01);
	RUN_TEST(test_sm83_opcode_0x02);
	RUN_TEST(test_sm83_opcode_0x03);
	RUN_TEST(test_sm83_opcode_0x04);
	RUN_TEST(test_sm83_opcode_0x05);
	RUN_TEST(test_sm83_opcode_0x06);
	RUN_TEST(test_sm83_opcode_0x07);
	RUN_TEST(test_sm83_opcode_0x08);
	RUN_TEST(test_sm83_opcode_0x09);
	RUN_TEST(test_sm83_opcode_0x0A);
	RUN_TEST(test_sm83_opcode_0x0B);
	RUN_TEST(test_sm83_opcode_0x0C);
	RUN_TEST(test_sm83_opcode_0x0D);
	RUN_TEST(test_sm83_opcode_0x0E);
	RUN_TEST(test_sm83_opcode_0x0F);
	RUN_TEST(test_sm83_opcode_0x11);
	RUN_TEST(test_sm83_opcode_0x12);
	RUN_TEST(test_sm83_opcode_0x13);
	RUN_TEST(test_sm83_opcode_0x14);
	RUN_TEST(test_sm83_opcode_0x15);
	RUN_TEST(test_sm83_opcode_0x16);
	RUN_TEST(test_sm83_opcode_0x17);
	RUN_TEST(test_sm83_opcode_0x18);
	RUN_TEST(test_sm83_opcode_0x19);
	RUN_TEST(test_sm83_opcode_0x1A);
	RUN_TEST(test_sm83_opcode_0x1B);
	RUN_TEST(test_sm83_opcode_0x1C);
	RUN_TEST(test_sm83_opcode_0x1D);
	RUN_TEST(test_sm83_opcode_0x1E);
	RUN_TEST(test_sm83_opcode_0x1F);
	RUN_TEST(test_sm83_opcode_0x20);
	RUN_TEST(test_sm83_opcode_0x21);
	RUN_TEST(test_sm83_opcode_0x22);
	RUN_TEST(test_sm83_opcode_0x23);
	RUN_TEST(test_sm83_opcode_0x24);
	RUN_TEST(test_sm83_opcode_0x25);
	RUN_TEST(test_sm83_opcode_0x26);
	RUN_TEST(test_sm83_opcode_0x27);
	RUN_TEST(test_sm83_opcode_0x28);
	RUN_TEST(test_sm83_opcode_0x29);
	RUN_TEST(test_sm83_opcode_0x2A);
	RUN_TEST(test_sm83_opcode_0x2B);
	RUN_TEST(test_sm83_opcode_0x2C);
	RUN_TEST(test_sm83_opcode_0x2D);
	RUN_TEST(test_sm83_opcode_0x2E);
	RUN_TEST(test_sm83_opcode_0x2F);
	RUN_TEST(test_sm83_opcode_0x30);
	RUN_TEST(test_sm83_opcode_0x31);
	RUN_TEST(test_sm83_opcode_0x32);
	RUN_TEST(test_sm83_opcode_0x33);
	RUN_TEST(test_sm83_opcode_0x34);
	RUN_TEST(test_sm83_opcode_0x35);
	RUN_TEST(test_sm83_opcode_0x36);
	RUN_TEST(test_sm83_opcode_0x37);
	RUN_TEST(test_sm83_opcode_0x38);
	RUN_TEST(test_sm83_opcode_0x39);
	RUN_TEST(test_sm83_opcode_0x3A);
	RUN_TEST(test_sm83_opcode_0x3B);
	RUN_TEST(test_sm83_opcode_0x3C);
	RUN_TEST(test_sm83_opcode_0x3D);
	RUN_TEST(test_sm83_opcode_0x3E);
	RUN_TEST(test_sm83_opcode_0x3F);
	RUN_TEST(test_sm83_opcode_0x40);
	RUN_TEST(test_sm83_opcode_0x41);
	RUN_TEST(test_sm83_opcode_0x42);
	RUN_TEST(test_sm83_opcode_0x43);
	RUN_TEST(test_sm83_opcode_0x44);
	RUN_TEST(test_sm83_opcode_0x45);
	RUN_TEST(test_sm83_opcode_0x46);
	RUN_TEST(test_sm83_opcode_0x47);
	RUN_TEST(test_sm83_opcode_0x48);
	RUN_TEST(test_sm83_opcode_0x49);
	RUN_TEST(test_sm83_opcode_0x4A);
	RUN_TEST(test_sm83_opcode_0x4B);
	RUN_TEST(test_sm83_opcode_0x4C);
	RUN_TEST(test_sm83_opcode_0x4D);
	RUN_TEST(test_sm83_opcode_0x4E);
	RUN_TEST(test_sm83_opcode_0x4F);
	RUN_TEST(test_sm83_opcode_0x50);
	RUN_TEST(test_sm83_opcode_0x51);
	RUN_TEST(test_sm83_opcode_0x52);
	RUN_TEST(test_sm83_opcode_0x53);
	RUN_TEST(test_sm83_opcode_0x54);
	RUN_TEST(test_sm83_opcode_0x55);
	RUN_TEST(test_sm83_opcode_0x56);
	RUN_TEST(test_sm83_opcode_0x57);
	RUN_TEST(test_sm83_opcode_0x58);
	RUN_TEST(test_sm83_opcode_0x59);
	RUN_TEST(test_sm83_opcode_0x5A);
	RUN_TEST(test_sm83_opcode_0x5B);
	RUN_TEST(test_sm83_opcode_0x5C);
	RUN_TEST(test_sm83_opcode_0x5D);
	RUN_TEST(test_sm83_opcode_0x5E);
	RUN_TEST(test_sm83_opcode_0x5F);
	RUN_TEST(test_sm83_opcode_0x60);
	RUN_TEST(test_sm83_opcode_0x61);
	RUN_TEST(test_sm83_opcode_0x62);
	RUN_TEST(test_sm83_opcode_0x63);
	RUN_TEST(test_sm83_opcode_0x64);
	RUN_TEST(test_sm83_opcode_0x65);
	RUN_TEST(test_sm83_opcode_0x66);
	RUN_TEST(test_sm83_opcode_0x67);
	RUN_TEST(test_sm83_opcode_0x68);
	RUN_TEST(test_sm83_opcode_0x69);
	RUN_TEST(test_sm83_opcode_0x6A);
	RUN_TEST(test_sm83_opcode_0x6B);
	RUN_TEST(test_sm83_opcode_0x6C);
	RUN_TEST(test_sm83_opcode_0x6D);
	RUN_TEST(test_sm83_opcode_0x6E);
	RUN_TEST(test_sm83_opcode_0x6F);
	RUN_TEST(test_sm83_opcode_0x70);
	RUN_TEST(test_sm83_opcode_0x71);
	RUN_TEST(test_sm83_opcode_0x72);
	RUN_TEST(test_sm83_opcode_0x73);
	RUN_TEST(test_sm83_opcode_0x74);
	RUN_TEST(test_sm83_opcode_0x75);
	RUN_TEST(test_sm83_opcode_0x76);
	RUN_TEST(test_sm83_opcode_0x77);
	RUN_TEST(test_sm83_opcode_0x78);
	RUN_TEST(test_sm83_opcode_0x79);
	RUN_TEST(test_sm83_opcode_0x7A);
	RUN_TEST(test_sm83_opcode_0x7B);
	RUN_TEST(test_sm83_opcode_0x7C);
	RUN_TEST(test_sm83_opcode_0x7D);
	RUN_TEST(test_sm83_opcode_0x7E);
	RUN_TEST(test_sm83_opcode_0x7F);
	RUN_TEST(test_sm83_opcode_0x80);
	RUN_TEST(test_sm83_opcode_0x81);
	RUN_TEST(test_sm83_opcode_0x82);
	RUN_TEST(test_sm83_opcode_0x83);
	RUN_TEST(test_sm83_opcode_0x84);
	RUN_TEST(test_sm83_opcode_0x85);
	RUN_TEST(test_sm83_opcode_0x86);
	RUN_TEST(test_sm83_opcode_0x87);
	RUN_TEST(test_sm83_opcode_0x88);
	RUN_TEST(test_sm83_opcode_0x89);
	RUN_TEST(test_sm83_opcode_0x8A);
	RUN_TEST(test_sm83_opcode_0x8B);
	RUN_TEST(test_sm83_opcode_0x8C);
	RUN_TEST(test_sm83_opcode_0x8D);
	RUN_TEST(test_sm83_opcode_0x8E);
	RUN_TEST(test_sm83_opcode_0x8F);
	RUN_TEST(test_sm83_opcode_0x90);
	RUN_TEST(test_sm83_opcode_0x91);
	RUN_TEST(test_sm83_opcode_0x92);
	RUN_TEST(test_sm83_opcode_0x93);
	RUN_TEST(test_sm83_opcode_0x94);
	RUN_TEST(test_sm83_opcode_0x95);
	RUN_TEST(test_sm83_opcode_0x96);
	RUN_TEST(test_sm83_opcode_0x97);
	RUN_TEST(test_sm83_opcode_0x98);
	RUN_TEST(test_sm83_opcode_0x99);
	RUN_TEST(test_sm83_opcode_0x9A);
	RUN_TEST(test_sm83_opcode_0x9B);
	RUN_TEST(test_sm83_opcode_0x9C);
	RUN_TEST(test_sm83_opcode_0x9D);
	RUN_TEST(test_sm83_opcode_0x9E);
	RUN_TEST(test_sm83_opcode_0x9F);
	RUN_TEST(test_sm83_opcode_0xA0);
	RUN_TEST(test_sm83_opcode_0xA1);
	RUN_TEST(test_sm83_opcode_0xA2);
	RUN_TEST(test_sm83_opcode_0xA3);
	RUN_TEST(test_sm83_opcode_0xA4);
	RUN_TEST(test_sm83_opcode_0xA5);
	RUN_TEST(test_sm83_opcode_0xA6);
	RUN_TEST(test_sm83_opcode_0xA7);
	RUN_TEST(test_sm83_opcode_0xA8);
	RUN_TEST(test_sm83_opcode_0xA9);
	RUN_TEST(test_sm83_opcode_0xAA);
	RUN_TEST(test_sm83_opcode_0xAB);
	RUN_TEST(test_sm83_opcode_0xAC);
	RUN_TEST(test_sm83_opcode_0xAD);
	RUN_TEST(test_sm83_opcode_0xAE);
	RUN_TEST(test_sm83_opcode_0xAF);
	RUN_TEST(test_sm83_opcode_0xB0);
	RUN_TEST(test_sm83_opcode_0xB1);
	RUN_TEST(test_sm83_opcode_0xB2);
	RUN_TEST(test_sm83_opcode_0xB3);
	RUN_TEST(test_sm83_opcode_0xB4);
	RUN_TEST(test_sm83_opcode_0xB5);
	RUN_TEST(test_sm83_opcode_0xB6);
	RUN_TEST(test_sm83_opcode_0xB7);
	RUN_TEST(test_sm83_opcode_0xB8);
	RUN_TEST(test_sm83_opcode_0xB9);
	RUN_TEST(test_sm83_opcode_0xBA);
	RUN_TEST(test_sm83_opcode_0xBB);
	RUN_TEST(test_sm83_opcode_0xBC);
	RUN_TEST(test_sm83_opcode_0xBD);
	RUN_TEST(test_sm83_opcode_0xBE);
	RUN_TEST(test_sm83_opcode_0xBF);
	RUN_TEST(test_sm83_opcode_0xC0);
	RUN_TEST(test_sm83_opcode_0xC1);
	RUN_TEST(test_sm83_opcode_0xC2);
	RUN_TEST(test_sm83_opcode_0xC3);
	RUN_TEST(test_sm83_opcode_0xC4);
	RUN_TEST(test_sm83_opcode_0xC5);
	RUN_TEST(test_sm83_opcode_0xC6);
	RUN_TEST(test_sm83_opcode_0xC7);
	RUN_TEST(test_sm83_opcode_0xC8);
	RUN_TEST(test_sm83_opcode_0xC9);
	RUN_TEST(test_sm83_opcode_0xCA);
	RUN_TEST(test_sm83_opcode_0xCC);
	RUN_TEST(test_sm83_opcode_0xCD);
	RUN_TEST(test_sm83_opcode_0xCE);
	RUN_TEST(test_sm83_opcode_0xCF);
	RUN_TEST(test_sm83_opcode_0xD0);
	RUN_TEST(test_sm83_opcode_0xD1);
	RUN_TEST(test_sm83_opcode_0xD2);
	RUN_TEST(test_sm83_opcode_0xD5);
	RUN_TEST(test_sm83_opcode_0xD6);
	RUN_TEST(test_sm83_opcode_0xD7);
	RUN_TEST(test_sm83_opcode_0xD8);
	RUN_TEST(test_sm83_opcode_0xD9);
	RUN_TEST(test_sm83_opcode_0xDA);
	RUN_TEST(test_sm83_opcode_0xDC);
	RUN_TEST(test_sm83_opcode_0xDE);
	RUN_TEST(test_sm83_opcode_0xDF);
	RUN_TEST(test_sm83_opcode_0xE0);
	RUN_TEST(test_sm83_opcode_0xE1);
	RUN_TEST(test_sm83_opcode_0xE2);
	RUN_TEST(test_sm83_opcode_0xE5);
	RUN_TEST(test_sm83_opcode_0xE6);
	RUN_TEST(test_sm83_opcode_0xE7);
	RUN_TEST(test_sm83_opcode_0xE8);
	RUN_TEST(test_sm83_opcode_0xE9);
	RUN_TEST(test_sm83_opcode_0xEA);
	RUN_TEST(test_sm83_opcode_0xEE);
	RUN_TEST(test_sm83_opcode_0xEF);
	RUN_TEST(test_sm83_opcode_0xF0);
	RUN_TEST(test_sm83_opcode_0xF1);
	RUN_TEST(test_sm83_opcode_0xF2);
	RUN_TEST(test_sm83_opcode_0xF3);
	RUN_TEST(test_sm83_opcode_0xF5);
	RUN_TEST(test_sm83_opcode_0xF6);
	RUN_TEST(test_sm83_opcode_0xF7);
	RUN_TEST(test_sm83_opcode_0xF8);
	RUN_TEST(test_sm83_opcode_0xF9);
	RUN_TEST(test_sm83_opcode_0xFA);
	RUN_TEST(test_sm83_opcode_0xFB);
	RUN_TEST(test_sm83_opcode_0xFE); 
	RUN_TEST(test_sm83_opcode_0xFF);
	return UNITY_END();
}