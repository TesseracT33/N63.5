module VR4300:Operation;

import :COP0;
import :COP1;
import :CPU;
import :Exceptions;
import :MMU;

import DebugOptions;
import Logging;

namespace VR4300
{
	template<u64 number_of_cycles>
	void AdvancePipeline()
	{
		static_assert(number_of_cycles > 0);
		p_cycle_counter += number_of_cycles;
		IncrementCountRegister<number_of_cycles>();
	}


	void CheckInterrupts()
	{
		bool interrupts_enabled = cop0_reg.status.ie;
		if (!interrupts_enabled) {
			return;
		}
		bool currently_handling_exception = cop0_reg.status.exl;
		if (currently_handling_exception) {
			return;
		}
		bool currently_handling_error = cop0_reg.status.erl;
		if (currently_handling_error) {
			return;
		}
		auto interrupt_pending = cop0_reg.cause.ip;
		auto interrupt_enable_mask = cop0_reg.status.im;
		if (interrupt_pending & interrupt_enable_mask) {
			SignalException<Exception::Interrupt>();
		}
	}


	/* Devices external to the CPU (e.g. the console's reset button) use this function to tell the CPU about interrupts. */
	void ClearInterruptPending(ExternalInterruptSource interrupt)
	{
		cop0_reg.cause.ip &= ~std::to_underlying(interrupt);
	}


	void FetchDecodeExecuteInstruction()
	{
		if constexpr (log_cpu_instructions) {
			current_instr_pc = pc;
		}
		u32 instr_code = FetchInstruction(pc);
		pc += 4;
		DecodeExecuteInstruction(instr_code);
	}


	void HlePif()
	{
		/* https://github.com/Dillonb/n64-resources/blob/master/bootn64.html */
		gpr.Set(20, 1);
		gpr.Set(22, 0x3F);
		gpr.Set(29, 0xA400'1FF0);
		cop0_reg.SetRaw(cop0_index_status, 0x2410'00E0);
		cop0_reg.SetRaw(cop0_index_config, 0x7006'E463);
		for (u64 i = 0; i < 0x1000; i += 4) {
			u64 src_addr = 0xFFFF'FFFF'B000'0000 + i;
			u64 dst_addr = 0xFFFF'FFFF'A400'0000 + i;
			WriteVirtual<u32>(dst_addr, ReadVirtual<u32>(src_addr));
		}
		pc = 0xFFFF'FFFF'A400'0040;
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
			if (cop0_reg.count.value == cop0_reg.compare.value) {
				SetInterrupt();
			}
		};

		if constexpr (number_of_cycles == 1) {
			cop0_reg.count.value = (cop0_reg.count.value + 1) & 0x1'FFFF'FFFF;
			CheckCompareReg();
		}
		else if constexpr (number_of_cycles == 2) {
			cop0_reg.count.value = (cop0_reg.count.value + 1) & 0x1'FFFF'FFFF;
			CheckCompareReg();
			cop0_reg.count.value = (cop0_reg.count.value + 1) & 0x1'FFFF'FFFF;
			CheckCompareReg();
		}
		else {
			if (cop0_reg.count.value + number_of_cycles <= 0x1'FFFF'FFFF) [[likely]] {
				if (cop0_reg.count.value > cop0_reg.compare.value) {
					cop0_reg.count.value += number_of_cycles;
				}
				else {
					cop0_reg.count.value += number_of_cycles;
					if (cop0_reg.count.value >= cop0_reg.compare.value) {
						SetInterrupt();
					}
				}
			}
			else {
				if (cop0_reg.count.value > cop0_reg.compare.value) {
					cop0_reg.count.value = number_of_cycles - (0x1'FFFF'FFFF - cop0_reg.count.value + 1);
					if (cop0_reg.count.value >= cop0_reg.compare.value) {
						SetInterrupt();
					}
				}
				else {
					cop0_reg.count.value = (cop0_reg.count.value + number_of_cycles) & 0x1'FFFF'FFFF;
					SetInterrupt();
				}
			}
		}
	}


	void InitializeRegisters()
	{
		std::memset(&gpr, 0, sizeof(gpr));
		std::memset(&fpr, 0, sizeof(fpr));
		gpr.Set(Reg::sp, 0xFFFF'FFFF'A400'1FF0);
		cop0_reg.SetRaw(cop0_index_index, 0x3F);
		cop0_reg.SetRaw(cop0_index_context, 0x007F'FFF0);
		cop0_reg.SetRaw(cop0_index_bad_v_addr, 0xFFFF'FFFF'FFFF'FFFF);
		cop0_reg.SetRaw(cop0_index_cause, 0xB000'007C);
		cop0_reg.SetRaw(cop0_index_epc, 0xFFFF'FFFF'FFFF'FFFF);
		cop0_reg.SetRaw(cop0_index_ll_addr, 0xFFFF'FFFF);
		cop0_reg.SetRaw(cop0_index_watch_lo, 0xFFFF'FFFB);
		cop0_reg.SetRaw(cop0_index_watch_hi, 0xF);
		cop0_reg.SetRaw(cop0_index_x_context, 0xFFFF'FFFF'FFFF'FFF0);
		cop0_reg.SetRaw(cop0_index_error_epc, 0xFFFF'FFFF'FFFF'FFFF);
	}


	void PowerOn(bool hle_pif)
	{
		exception_has_occurred = false;
		jump_is_pending = false;

		InitializeRegisters();
		InitializeFPU();
		InitializeMMU();

		if (hle_pif) {
			HlePif();
		}
		else {
			if constexpr (skip_boot_rom) {
				HlePif();
			}
			else {
				SignalException<Exception::ColdReset>();
				HandleException();
			}
		}
	}


	void PrepareJump(u64 target_address)
	{
		jump_is_pending = true;
		instructions_until_jump = 1;
		addr_to_jump_to = target_address;
		pc_is_inside_branch_delay_slot = true;
	}


	void Reset()
	{
		exception_has_occurred = false;
		jump_is_pending = false;
		SignalException<Exception::SoftReset>();
		HandleException();
	}


	void Run(uint cycles_to_run)
	{
		p_cycle_counter = 0;

		while (p_cycle_counter < cycles_to_run) {
			if (jump_is_pending) {
				if (instructions_until_jump-- == 0) {
					pc = addr_to_jump_to;
					jump_is_pending = false;
					pc_is_inside_branch_delay_slot = false;
				}
			}

			FetchDecodeExecuteInstruction();

			if (exception_has_occurred) {
				HandleException();
			}
		}
	}


	void SetInterruptPending(ExternalInterruptSource interrupt)
	{
		cop0_reg.cause.ip |= std::to_underlying(interrupt);
		CheckInterrupts();
	}


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