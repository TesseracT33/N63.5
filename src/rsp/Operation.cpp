module RSP:Operation;

import :Interface;

import DebugOptions;
import DMA;
import Logging;
import MI;

namespace RSP
{
	void AdvancePipeline(u64 cycles)
	{
		p_cycle_counter += cycles;
	}


	void FetchDecodeExecuteInstruction()
	{
		if constexpr (log_rsp_instructions) {
			current_instr_pc = pc;
		}
		u32 instr_code;
		std::memcpy(&instr_code, imem.data() + pc, 4); /* TODO: can pc be misaligned? */
		instr_code = std::byteswap(instr_code);
		pc = (pc + 4) & 0xFFF;
		DecodeExecuteInstruction(instr_code);
	}


	u8* GetPointerToMemory(u32 addr)
	{
		return memory_ptrs[bool(addr & 0x1000)] + (addr & 0xFFF);
	}


	void NotifyIllegalInstrCode(u32 instr_code)
	{
		std::cout << std::format("Illegal RSP instruction code {:08X} encountered.\n", instr_code);
	}


	void PowerOn()
	{
		jump_is_pending = false;
		pc = 0;
		dmem.fill(0);
		imem.fill(0);
		std::memset(&sp, 0, sizeof(sp));
		sp.status.halted = true;
	}


	void PrepareJump(u32 target_address)
	{
		jump_is_pending = true;
		instructions_until_jump = 1;
		addr_to_jump_to = target_address;
	}


	template<std::signed_integral Int>
	Int ReadDMEM(u32 addr)
	{
		/* Addr may be misaligned and the read can go out of bounds */
		Int ret;
		for (size_t i = 0; i < sizeof(Int); ++i) {
			*((u8*)(&ret) + sizeof(Int) - i - 1) = dmem[(addr + i) & 0xFFF];
		}
		return ret;
	}


	template<std::signed_integral Int>
	Int ReadMemoryCpu(u32 addr)
	{ /* CPU precondition; the address is always aligned */
		if (addr < 0x0404'0000) {
			Int ret;
			std::memcpy(&ret, memory_ptrs[bool(addr & 0x1000)] + (addr & 0xFFF), sizeof(Int));
			return std::byteswap(ret);
		}
		else if constexpr (sizeof(Int) == 4) {
			return ReadReg(addr);
		}
		else {
			Logging::LogMisc(std::format(
				"Attempted to read RSP memory region at address ${:08X} for sized int {}",
				addr, sizeof(Int)));
			return {};
		}
	}


	u64 RspReadCommandByteswapped(u32 addr)
	{
		/* The address may be unaligned */
		u64 command;
		for (int i = 0; i < 8; ++i) {
			*((u8*)(&command) + i) = dmem[(addr + 7 - i) & 0xFFF];
		}
		return command;
	}


	u64 Run(u64 rsp_cycles_to_run)
	{
		if (sp.status.halted) {
			return 0;
		}
		p_cycle_counter = 0;
		while (p_cycle_counter < rsp_cycles_to_run) {
			if (jump_is_pending) {
				if (instructions_until_jump-- == 0) {
					pc = addr_to_jump_to;
					jump_is_pending = false;
				}
			}
			FetchDecodeExecuteInstruction();
			if (sp.status.sstep || sp.status.halted) {
				if (sp.status.sstep) {
					sp.status.halted = true;
				}
				return p_cycle_counter <= rsp_cycles_to_run ? 0 : p_cycle_counter - rsp_cycles_to_run;
			}
		}
		return p_cycle_counter - rsp_cycles_to_run;
	}


	template<std::signed_integral Int>
	void WriteDMEM(u32 addr, Int data)
	{
		/* Addr may be misaligned and the write can go out of bounds */
		for (size_t i = 0; i < sizeof(Int); ++i) {
			dmem[(addr + i) & 0xFFF] = *((u8*)(&data) + sizeof(Int) - i - 1);
		}
	}


	template<size_t num_bytes>
	void WriteMemoryCpu(u32 addr, std::signed_integral auto data)
	{ /* CPU precondition; the address may be misaligned, but then, 'number_of_bytes' is set so that it the write goes only to the next boundary. */
		if (addr < 0x0404'0000) {
			data = std::byteswap(data);
			std::memcpy(memory_ptrs[bool(addr & 0x1000)] + (addr & 0xFFF), &data, num_bytes);
		}
		else if constexpr (num_bytes == 4) {
			WriteReg(addr, data);
		}
		else {
			Logging::LogMisc(std::format(
				"Attempted to write to RSP memory region at address ${:08X} for sized int {}",
				addr, num_bytes));
		}
	}


	template s8 ReadDMEM<s8>(u32);
	template s16 ReadDMEM<s16>(u32);
	template s32 ReadDMEM<s32>(u32);
	template s64 ReadDMEM<s64>(u32);
	template void WriteDMEM<s8>(u32, s8);
	template void WriteDMEM<s16>(u32, s16);
	template void WriteDMEM<s32>(u32, s32);
	template void WriteDMEM<s64>(u32, s64);

	template s8 ReadMemoryCpu<s8>(u32);
	template s16 ReadMemoryCpu<s16>(u32);
	template s32 ReadMemoryCpu<s32>(u32);
	template s64 ReadMemoryCpu<s64>(u32);

	template void WriteMemoryCpu<1>(u32, s8);
	template void WriteMemoryCpu<1>(u32, s16);
	template void WriteMemoryCpu<1>(u32, s32);
	template void WriteMemoryCpu<1>(u32, s64);
	template void WriteMemoryCpu<2>(u32, s16);
	template void WriteMemoryCpu<2>(u32, s32);
	template void WriteMemoryCpu<2>(u32, s64);
	template void WriteMemoryCpu<3>(u32, s32);
	template void WriteMemoryCpu<3>(u32, s64);
	template void WriteMemoryCpu<4>(u32, s32);
	template void WriteMemoryCpu<4>(u32, s64);
	template void WriteMemoryCpu<5>(u32, s64);
	template void WriteMemoryCpu<6>(u32, s64);
	template void WriteMemoryCpu<7>(u32, s64);
	template void WriteMemoryCpu<8>(u32, s64);
}