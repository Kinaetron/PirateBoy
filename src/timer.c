#include "timer.h"

static uint32_t divider_cycles = 0;
static uint32_t previous_register_count = 0;

static void divider_register_incrementer(CPU_Memory* memory, uint32_t cycles)
{
	if (cycles >= DIVER_INCREMENT) 
	{
		if (memory->input_output[DIVIDER_REGISTER] != previous_register_count) 
		{
			memory->input_output[DIVIDER_REGISTER] = 0;
			divider_cycles = 0;

			return;
		}

		while (divider_cycles >= DIVER_INCREMENT)
		{
			divider_cycles -= DIVER_INCREMENT;

			memory->input_output[DIVIDER_REGISTER]++;
			previous_register_count = memory->input_output[DIVIDER_REGISTER];
		}
	}
}

void timer_step(CPU_Memory* memory, uint8_t cycles)
{
	divider_register_incrementer(memory, cycles);
}