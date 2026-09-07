#include "timer.h"
#include <stdbool.h>

static uint32_t tima_cycles = 0;
static uint32_t divider_cycles = 0;
static const uint16_t clock_values[4] = { 1024, 16, 64, 256 };

static void divider_register_incrementer(CPU_Memory* memory, uint32_t cycles)
{
	divider_cycles += cycles;

	while (divider_cycles >= DIVIDER_INCREMENT)
	{
		divider_cycles -= DIVIDER_INCREMENT;
		memory_divider_register_incrementer(memory);
	}
}

static uint16_t tima_clock_type(CPU_Memory* memory)
{
	uint16_t clock_value = (memory_read(memory, TIMER_CONTROL) & 0x03);

	return clock_value;
}

static bool tima_timer_enabled(CPU_Memory* memory)
{
	uint8_t tac_register = memory_read(memory, TIMER_CONTROL);

	return (bool)((tac_register >> 2) & 0x01);
}

static void timer_counter_incrementer(CPU_Memory* memory, uint32_t cycles)
{
	if (!tima_timer_enabled(memory)) {
		return;
	}

	tima_cycles += cycles;

	uint16_t increment_value = clock_values[tima_clock_type(memory)];

	while (tima_cycles >= increment_value)
	{
		tima_cycles -= increment_value;

		uint8_t current_count = memory_read(memory, TIMER_COUNTER);
		uint8_t new_count = current_count + 1;

		memory_write(memory, TIMER_COUNTER, new_count);

		if (new_count == 0x00)
		{
			uint8_t timer_modulo = memory_read(memory, TIMER_MODULO);
			memory_write(memory, TIMER_COUNTER, timer_modulo);
			set_if_interrupt(memory, Timer, true);
		}
	}
}

void timer_reset_divider(void) {
	divider_cycles = 0;
}

void timer_step(CPU_Memory* memory, uint8_t cycles) 
{
	divider_register_incrementer(memory, cycles);
	timer_counter_incrementer(memory, cycles);
}