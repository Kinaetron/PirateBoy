#include "cpu.h"
#include "interrupt.h"

static const uint16_t interrupt_vectors[5] =
{
	0x0040, // VBlank
	0x0048, // LCD STAT
	0x0050, // Timer
	0x0058, // Serial
	0x0060  // Joypad
};

static uint8_t handle_interrupts(CPU_Memory* memory)
{
	if (cpu_interrupt_master_enable())
	{
		if (is_pending(memory) != 0)
		{
			for (int i = 0; i < 5; i++)
			{
				if (get_if_interrupt(memory, (Interrupt_Flag)i) &&
					get_ie_interrupt(memory, (Interrupt_Flag)i))
				{
					set_if_interrupt(memory, (Interrupt_Flag)i, false);
					cpu_set_interrupt_master_enable(false);

					memory16 return_address;
					return_address.value = memory->program_counter.value;

					write_byte(memory, &memory->stack_pointer, return_address.high);
					write_byte(memory, &memory->stack_pointer, return_address.low);

					memory->program_counter.value = interrupt_vectors[i];

					return 20;
				}
			}
		}
	}

	return 0;
}
