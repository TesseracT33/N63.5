module VR4300:Operation;

import :COP0;
import :CPU;
import :MMU;

namespace VR4300
{
	void Run(const int cycles_to_run)
	{
		p_cycle_counter = 0;

		while (p_cycle_counter < cycles_to_run)
		{
			ExecuteInstruction();
		}
	}

	void Reset()
	{
		p_cycle_counter = 0;
		jump_is_pending = false;
	}

	void ExecuteInstruction() /* todo: bad name for now */
	{
		if (jump_is_pending)
		{
			if (instructions_until_jump == 0)
			{
				PC = addr_to_jump_to;
				jump_is_pending = false;
			}
			else instructions_until_jump--;
		}

		const u32 instr_code = cpu_read_mem<u32>(PC);
		PC += 4;
		DecodeAndExecuteInstruction(instr_code);

		DecrementRandomRegister();
	}

	void EnterKernelMode()
	{

	}

	void DisableInterrupts()
	{

	}
}