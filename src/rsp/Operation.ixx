export module RSP:Operation;

import :ScalarUnit;
import :VectorUnit;

import Util;

import <array>;
import <bit>;
import <concepts>;
import <cstring>;
import <format>;
import <iostream>;
import <string>;
import <string_view>;

namespace RSP
{
	export
	{
		u8* GetPointerToMemory(u32 addr);
		void PowerOn();
		u32 RdpReadCommand(u32 addr);
		u64 RdpReadCommandByteswapped(u32 addr);
		u64 Run(u64 rsp_cycles_to_run);

		template<std::signed_integral Int>
		Int ReadMemoryCpu(u32 addr);

		template<size_t access_size>
		void WriteMemoryCpu(u32 addr, s64 data);
	}

	void AdvancePipeline(u64 cycles);
	void DecodeExecuteCop0Instruction();
	void DecodeExecuteCop2Instruction();
	void DecodeExecuteInstruction(u32 instr_code);
	void DecodeExecuteRegimmInstruction();
	void DecodeExecuteSpecialInstruction();
	template<ScalarInstruction> void ExecuteScalarInstruction();
	template<VectorInstruction> void ExecuteVectorInstruction();
	void FetchDecodeExecuteInstruction();
	void NotifyIllegalInstrCode(u32 instr_code);
	void PrepareJump(u32 target_address);
	template<std::signed_integral Int> Int ReadDMEM(u32 addr);
	template<std::signed_integral Int> void WriteDMEM(u32 addr, Int data);

	bool in_branch_delay_slot;
	bool jump_is_pending;
	uint pc;
	uint p_cycle_counter;
	uint instructions_until_jump;
	uint addr_to_jump_to;

	constinit std::array<u8, 0x2000> mem; /* 0 - $FFF: data memory; $1000 - $1FFF: instruction memory */

	constinit inline u8* const dmem = mem.data();
	constinit inline u8* const imem = mem.data() + 0x1000;

	/* Debugging */
	u32 current_instr_pc;
	std::string_view current_instr_name;
	std::string current_instr_log_output;
}