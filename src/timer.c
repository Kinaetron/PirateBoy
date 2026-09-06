#include "timer.h"

static uint32_t divider_cycles = 0;

static void divider_register_incrementer(CPU_Memory* memory, uint32_t cycles)
{
	divider_cycles += cycles;

	while (divider_cycles >= DIVIDER_INCREMENT)
	{
		divider_cycles -= DIVIDER_INCREMENT;
		memory_divider_register_incrementer(memory);
	}
}

void timer_reset_divider(void) {
	divider_cycles = 0;
}

void timer_step(CPU_Memory* memory, uint8_t cycles) {
	divider_register_incrementer(memory, cycles);
}