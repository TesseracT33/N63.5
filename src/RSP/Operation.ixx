export module RSP:Operation;

import Memory;
import N64;
import NumericalTypes;

import <array>;
import <bit>;
import <concepts>;
import <string>;
import <string_view>;

namespace RSP
{
	std::array<u8, 0x1000> dmem{}; /* data memory */
	std::array<u8, 0x1000> imem{}; /* instruction memory */

	void DecodeExecuteInstruction(u32 instr_code);
	void FetchDecodeExecuteInstruction();
	void PrepareJump(u32 target_address);

	template<u64 number_of_cycles>
	void AdvancePipeline();

	template<std::integral Int>
	Int ReadDMEM(u32 addr);

	template<std::integral Int>
	void WriteDMEM(u32 addr, Int data);

	bool jump_is_pending = false;
	unsigned pc;
	unsigned p_cycle_counter;
	unsigned instructions_until_jump = 0;
	unsigned addr_to_jump_to;

	/* Debugging */
	u32 current_instr_pc;
	std::string_view current_instr_name;
	std::string current_instr_log_output;

	export
	{
		u8* GetPointerToMemory(u32 addr);

		template<std::integral Int>
		Int CPUReadMemory(u32 addr);

		template<std::size_t number_of_bytes>
		void CPUWriteMemory(u32 addr, auto data);

		template<std::integral Int>
		Int CPUReadRegister(u32 addr);

		template<std::size_t number_of_bytes>
		void CPUWriteRegister(u32 addr, auto data);

		void Run(unsigned cycles_to_run);
	}
}