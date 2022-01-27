module VR4300:Operation;

import :COP0;
import :CPU;
import :Exceptions;
import :MMU;

import <cassert>;

namespace VR4300
{
	void Run(const int cycles_to_run)
	{
		p_cycle_counter = 0;

		while (p_cycle_counter < cycles_to_run)
		{
			ExecuteInstruction();
			DecrementRandomRegister();
			if (exception_has_occurred)
				HandleException();
		}
	}

	void Reset()
	{
		jump_is_pending = false;

		SignalException<Exception::SoftReset>();
		HandleException();
	}

	void PowerOn(const bool hle_pif)
	{
		exception_has_occurred = false;
		jump_is_pending = false;

		if (hle_pif)
		{
			HLE_PIF();
		}
		else
		{
			SignalException<Exception::ColdReset>();
			HandleException();
		}
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
	}

	void EnterKernelMode()
	{

	}

	void DisableInterrupts()
	{

	}

	void HLE_PIF()
	{
		/* https://github.com/Dillonb/n64-resources/blob/master/bootn64.html */
		GPR.Set(20, 1);
		GPR.Set(22, 0x3F);
		GPR.Set(29, 0xA400'1FF0);
		COP0_reg.Set(1, 0x0000'001F);
		COP0_reg.Set(12, 0x7040'0004);
		COP0_reg.Set(15, 0x0000'0B00);
		COP0_reg.Set(16, 0x0006'E463);

		cpu_write_mem<u32>(0x0430'0004, 0x0101'0101);
		for (unsigned i = 0; i < 0x1000; i++) /* no clue if some kind of DMA */
			cpu_write_mem<u32>(0xA400'0000 + i, cpu_read_mem<u32>(0xB000'0000 + i));

		PC = 0xA4000040;
	}
}