module VR4300:Operation;

import :COP0;
import :COP1;
import :CPU;
import :Exceptions;
import :MMU;
import :Recompiler;

import BuildOptions;
import Logging;
import RDRAM;

namespace VR4300
{
	void AddInitialEvents()
	{
		ReloadCountCompareEvent<true>();
	}


	void AdvancePipeline(u64 cycles)
	{
		p_cycle_counter += cycles;
		cop0.count += cycles;
		if constexpr (recompile_cpu) {
			Recompiler::current_block_cycle_counter += cycles;
		}
	}


	void CheckInterrupts()
	{
		/* on real HW, these conditions are checked every cycle */
		if (!cop0.status.ie || cop0.status.exl || cop0.status.erl) {
			return; /* interrupts disabled, or already handling exception or error */
		}
		if (cop0.cause.ip & cop0.status.im) {
			SignalException<Exception::Interrupt>();
		}
	}


	/* Devices external to the CPU (e.g. the console's reset button) use this function to tell the CPU about interrupts. */
	void ClearInterruptPending(ExternalInterruptSource interrupt)
	{
		cop0.cause.ip &= ~std::to_underlying(interrupt);
	}


	void FetchDecodeExecuteInstruction()
	{
		u32 instr_code = FetchInstruction(pc);
		pc += 4;
		DecodeExecuteInstruction(instr_code);
	}


	u64 GetElapsedCycles()
	{
		return p_cycle_counter;
	}


	void HlePif()
	{
		/* https://github.com/Dillonb/n64-resources/blob/master/bootn64.html */
		gpr.Set(20, 1);
		gpr.Set(22, 0x3F);
		gpr.Set(29, 0xA400'1FF0);
		cop0.SetRaw(cop0_index_status, 0x3400'0000);
		cop0.SetRaw(cop0_index_config, 0x7006'E463);
		for (u64 i = 0; i < 0x1000; i += 4) {
			u64 src_addr = 0xFFFF'FFFF'B000'0000 + i;
			u64 dst_addr = 0xFFFF'FFFF'A400'0000 + i;
			WriteVirtual<4>(dst_addr, ReadVirtual<s32>(src_addr));
		}
		pc = 0xFFFF'FFFF'A400'0040;
	}


	void InitializeRegisters()
	{
		std::memset(&gpr, 0, sizeof(gpr));
		std::memset(&fpr, 0, sizeof(fpr));
		gpr.Set(Reg::sp, 0xFFFF'FFFF'A400'1FF0);
		cop0.SetRaw(cop0_index_index, 0x3F);
		cop0.SetRaw(cop0_index_config, 0x7006'E463);
		cop0.SetRaw(cop0_index_context, 0x007F'FFF0);
		cop0.SetRaw(cop0_index_bad_v_addr, 0xFFFF'FFFF'FFFF'FFFF);
		cop0.SetRaw(cop0_index_cause, 0xB000'007C);
		cop0.SetRaw(cop0_index_epc, 0xFFFF'FFFF'FFFF'FFFF);
		cop0.SetRaw(cop0_index_status, 0x3400'0000);
		cop0.SetRaw(cop0_index_ll_addr, 0xFFFF'FFFF);
		cop0.SetRaw(cop0_index_watch_lo, 0xFFFF'FFFB);
		cop0.SetRaw(cop0_index_watch_hi, 0xF);
		cop0.SetRaw(cop0_index_x_context, 0xFFFF'FFFF'FFFF'FFF0);
		cop0.SetRaw(cop0_index_error_epc, 0xFFFF'FFFF'FFFF'FFFF);
	}


	void NotifyIllegalInstrCode(u32 instr_code)
	{
		Log(std::format("Illegal CPU instruction code {:08X} encountered.\n", instr_code));
	}


	void PowerOn(bool hle_pif)
	{
		rdram_ptr = RDRAM::GetPointerToMemory();
		exception_has_occurred = false;
		jump_is_pending = false;

		InitializeRegisters();
		InitializeFpu();
		InitializeMMU();

		if (hle_pif || skip_boot_rom) {
			HlePif();
		}
		else {
			SignalException<Exception::ColdReset>();
			HandleException();
		}

		if constexpr (recompile_cpu) {
			Recompiler::Initialize();
		}
	}


	void PrepareJump(u64 target_address)
	{
		jump_is_pending = true;
		instructions_until_jump = 1;
		addr_to_jump_to = target_address;
	}


	void Reset()
	{
		exception_has_occurred = false;
		jump_is_pending = false;
		SignalException<Exception::SoftReset>();
		HandleException();
	}


	u64 Run(u64 cpu_cycles_to_run)
	{
		p_cycle_counter = 0;
		while (p_cycle_counter < cpu_cycles_to_run) {
			if (jump_is_pending) {
				if (instructions_until_jump-- == 0) {
					pc = addr_to_jump_to;
					jump_is_pending = false;
					in_branch_delay_slot = false;
				}
				else {
					in_branch_delay_slot = true;
				}
			}
			FetchDecodeExecuteInstruction();
			if (exception_has_occurred) {
				HandleException();
			}
		}
		return p_cycle_counter - cpu_cycles_to_run;
	}


	void SetInterruptPending(ExternalInterruptSource interrupt)
	{
		cop0.cause.ip |= std::to_underlying(interrupt);
		CheckInterrupts();
	}
}