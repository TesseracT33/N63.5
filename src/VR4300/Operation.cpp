module VR4300:Operation;

import :COP0;
import :COP1;
import :CPU;
import :Exceptions;
import :MMU;
import :Registers;

import DebugOptions;
import Logging;

namespace VR4300
{
	void Run(const unsigned cycles_to_run)
	{
		p_cycle_counter = 0;

		while (p_cycle_counter < cycles_to_run)
		{
			if (jump_is_pending)
			{
				if (instructions_until_jump-- == 0)
				{
					pc = addr_to_jump_to;
					jump_is_pending = false;
					pc_is_inside_branch_delay_slot = false;
				}
			}

			FetchDecodeExecuteInstruction();

			if (exception_has_occurred)
				HandleException();
		}
	}


	void Reset()
	{
		exception_has_occurred = false;
		jump_is_pending = false;

		SignalException<Exception::SoftReset>();
		HandleException();
	}


	void PowerOn(const bool hle_pif)
	{
		exception_has_occurred = false;
		jump_is_pending = false;

		InitializeRegisters();
		InitializeFPU();
		InitializeMMU();

		if (hle_pif)
		{
			HLE_PIF();
		}
		else
		{
			if constexpr (skip_boot_rom)
			{
				HLE_PIF();
			}
			else
			{
				SignalException<Exception::ColdReset>();
				HandleException();
			}
		}
	}


	void FetchDecodeExecuteInstruction() /* todo: bad name for now */
	{
		if constexpr (log_cpu_instructions)
		{
			current_instr_pc = pc;
		}
		const u32 instr_code = FetchInstruction(pc);
		pc += 4;
		DecodeExecuteInstruction(instr_code);
	}


	void CheckInterrupts()
	{
		const bool interrupts_are_enabled = cop0_reg.status.ie;
		if (!interrupts_are_enabled)
			return;

		const bool currently_handling_exception = cop0_reg.status.exl;
		if (currently_handling_exception)
			return;

		const bool currently_handling_error = cop0_reg.status.erl;
		if (currently_handling_error)
			return;

		const s32 interrupt_pending = cop0_reg.cause.ip;
		const s32 interrupt_enable_mask = cop0_reg.status.im;
		const bool interrupts_are_pending = interrupt_pending & interrupt_enable_mask & 0xFF; /* TODO: Unsure if $FF is needed */
		if (interrupts_are_pending)
			SignalException<Exception::Interrupt>();
	}


	/* Devices external to the CPU (e.g. the console's reset button) use this function to tell the CPU about interrupts. */
	template<ExternalInterruptSource interrupt>
	void ClearInterruptPending()
	{
		cop0_reg.cause.ip &= ~std::to_underlying(interrupt);
	}


	template<ExternalInterruptSource interrupt>
	void SetInterruptPending()
	{
		cop0_reg.cause.ip |= std::to_underlying(interrupt);
		CheckInterrupts();
	}


	void HLE_PIF()
	{
		/* https://github.com/Dillonb/n64-resources/blob/master/bootn64.html */
		gpr.Set(20, 1);
		gpr.Set(22, 0x3F);
		gpr.Set(29, 0xA400'1FF0);
		cop0_reg.SetRaw(cop0_index_status, 0x2410'00E0);
		cop0_reg.SetRaw(cop0_index_config, 0x7006'E463);
		
		for (int i = 0; i < 0x1000; i += 4) /* no clue if some kind of DMA */
		{
			const s32 src_addr = s32(0xB000'0000 + i);
			const s32 dest_addr = s32(0xA400'0000 + i);
			WriteVirtual<s32>(dest_addr, ReadVirtual<s32>(src_addr));
		}

		pc = 0xFFFF'FFFF'A400'0040;
	}


	template<u64 number_of_cycles>
	void AdvancePipeline()
	{
		static_assert(number_of_cycles > 0);

		p_cycle_counter += number_of_cycles;
		IncrementCountRegister<number_of_cycles>();
	}


	template<u64 number_of_cycles>
	void IncrementCountRegister()
	{
		static_assert(number_of_cycles > 0);

		/* The 32-bit 'count' register is supposed to be incremented every other PCycle, and then compared against the 32-bit 'compare' register.
		   If they match, and interrupt is fired. Here, we make both registers 64 bits, and increment 'count' every PCycle. */

		auto SetInterrupt = [&] {
			cop0_reg.cause.ip |= 0x80;
			CheckInterrupts();
		};

		auto CheckCompareReg = [&] {
			if (cop0_reg.count.value == cop0_reg.compare.value)
				SetInterrupt();
		};

		if constexpr (number_of_cycles == 1)
		{
			cop0_reg.count.value = (cop0_reg.count.value + 1) & 0x1'FFFF'FFFF;
			CheckCompareReg();
		}
		else if constexpr (number_of_cycles == 2)
		{
			cop0_reg.count.value = (cop0_reg.count.value + 1) & 0x1'FFFF'FFFF;
			CheckCompareReg();
			cop0_reg.count.value = (cop0_reg.count.value + 1) & 0x1'FFFF'FFFF;
			CheckCompareReg();
		}
		else
		{
			if (cop0_reg.count.value + number_of_cycles <= 0x1'FFFF'FFFF) [[likely]]
			{
				if (cop0_reg.count.value > cop0_reg.compare.value)
				{
					cop0_reg.count.value += number_of_cycles;
				}
				else
				{
					cop0_reg.count.value += number_of_cycles;
					if (cop0_reg.count.value >= cop0_reg.compare.value)
					{
						SetInterrupt();
					}
				}
			}
			else
			{
				if (cop0_reg.count.value > cop0_reg.compare.value)
				{
					cop0_reg.count.value = number_of_cycles - (0x1'FFFF'FFFF - cop0_reg.count.value + 1);
					if (cop0_reg.count.value >= cop0_reg.compare.value)
					{
						SetInterrupt();
					}
				}
				else
				{
					cop0_reg.count.value = (cop0_reg.count.value + number_of_cycles) & 0x1'FFFF'FFFF;
					SetInterrupt();
				}
			}
		}
	}


	void PrepareJump(const u64 target_address)
	{
		jump_is_pending = true;
		instructions_until_jump = 1;
		addr_to_jump_to = target_address;
		pc_is_inside_branch_delay_slot = true;
	}


	template void ClearInterruptPending<ExternalInterruptSource::MI>();
	template void ClearInterruptPending<ExternalInterruptSource::Cartridge>();
	template void ClearInterruptPending<ExternalInterruptSource::Reset>();
	template void ClearInterruptPending<ExternalInterruptSource::IndyRead>();
	template void ClearInterruptPending<ExternalInterruptSource::IndyWrite>();
	template void SetInterruptPending<ExternalInterruptSource::MI>();
	template void SetInterruptPending<ExternalInterruptSource::Cartridge>();
	template void SetInterruptPending<ExternalInterruptSource::Reset>();
	template void SetInterruptPending<ExternalInterruptSource::IndyRead>();
	template void SetInterruptPending<ExternalInterruptSource::IndyWrite>();

	template void AdvancePipeline<1>();
	template void AdvancePipeline<2>();
	template void AdvancePipeline<3>();
	template void AdvancePipeline<5>();
	template void AdvancePipeline<8>();
	template void AdvancePipeline<29>();
	template void AdvancePipeline<37>();
	template void AdvancePipeline<58>();
	template void AdvancePipeline<66>();
	template void AdvancePipeline<69>();

	template void IncrementCountRegister<1>();
	template void IncrementCountRegister<2>();
	template void IncrementCountRegister<3>();
	template void IncrementCountRegister<5>();
	template void IncrementCountRegister<8>();
	template void IncrementCountRegister<29>();
	template void IncrementCountRegister<37>();
	template void IncrementCountRegister<58>();
	template void IncrementCountRegister<66>();
	template void IncrementCountRegister<69>();
}