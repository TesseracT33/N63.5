export module RSP:Operation;

import Memory;
import N64;
import NumericalTypes;

import <array>;
import <bit>;
import <concepts>;
import <cstring>;
import <string>;
import <string_view>;

namespace RSP
{
	export
	{
		template<std::integral Int>
		Int CPUReadMemory(u32 addr);

		template<size_t number_of_bytes>
		void CPUWriteMemory(u32 addr, auto data);

		u8* GetPointerToMemory(u32 addr);
		void PowerOn();
		void Run(uint cycles_to_run);
	}

	void AdvancePipeline(u64 cycles);
	void DecodeExecuteInstruction(u32 instr_code);
	void FetchDecodeExecuteInstruction();
	void PrepareJump(u32 target_address);

	template<std::integral Int>
	Int ReadDMEM(u32 addr);

	template<std::integral Int>
	void WriteDMEM(u32 addr, Int data);

	bool halted = false;
	bool jump_is_pending = false;
	bool single_step_mode = false;
	uint pc;
	uint p_cycle_counter;
	uint instructions_until_jump = 0;
	uint addr_to_jump_to;

	/* Debugging */
	u32 current_instr_pc;
	std::string_view current_instr_name;
	std::string current_instr_log_output;

	std::array<u8, 0x1000> dmem; /* data memory */
	std::array<u8, 0x1000> imem; /* instruction memory */

	constexpr std::array memory_ptrs = { dmem.data(), imem.data() };
}