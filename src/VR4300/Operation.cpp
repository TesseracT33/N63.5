module VR4300:Operation;

import :COP0;
import :CPU;
import :Exceptions;
import :MMU;
import :Registers;

import <cassert>;

namespace VR4300
{
	void Run(const unsigned cycles_to_run)
	{
		p_cycle_counter = 0;

		while (p_cycle_counter < cycles_to_run)
		{
			if (jump_is_pending)
			{
				if (instructions_until_jump == 0)
				{
					pc = addr_to_jump_to;
					jump_is_pending = false;
				}
				else instructions_until_jump--;
			}

			ExecuteInstruction();

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


		const u32 instr_code = FetchInstruction(pc);
		pc += 4;
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
		gpr.Set(20, 1);
		gpr.Set(22, 0x3F);
		gpr.Set(29, 0xA400'1FF0);
		cop0_reg.Set(1, 0x0000'001F); /* random */
		cop0_reg.Set(12, 0x7040'0004); /* status */
		cop0_reg.Set(15, 0x0000'0B00); /* pr_id */
		cop0_reg.Set(16, 0x0006'E463); /* config */
		cop0_reg.status.NotifyCpuAfterWrite();
		cop0_reg.config.NotifyCpuAfterWrite();

		for (unsigned i = 0; i < 0x1000; i += 4) /* no clue if some kind of DMA */
			WriteVirtual<u32>(0xA400'0000 + i, ReadVirtual<u32>(0xB000'0000 + i));

		pc = 0xA400'0040;
	}


	void AdvancePipeline(const unsigned number_of_cycles)
	{
		p_cycle_counter += number_of_cycles;

		/* This register is incremented every other PCycle. */
		cop0_reg.count.value += number_of_cycles >> 1;
		if (cop0_reg.count.value == cop0_reg.compare.value)
		{
			cop0_reg.cause.ip7 = 1;
			/* TODO: fire interrupt */
		}

		/* TODO: decrement the 'random' register */
	}
}