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

static void raw_memory_write(CPU_Memory* memory, uint16_t address, uint8_t value) {
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

	memory->flat[INTERRUPT_FLAG_ADDR] = json_u8(state, "ie");

	//cpu_interrupt_master_enable(cJSON_GetObjectItem(state, "ime")->valueint != 0);

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

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_sm83_opcode_0x00);
	return UNITY_END();
}