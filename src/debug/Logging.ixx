export module Logging;

import NumericalTypes;

import <format>;
import <fstream>;

namespace Logging
{
	export void LogMemoryRead(u32 physical_address, auto memory_value);
	export void LogMemoryWrite(u32 physical_address, auto memory_value);
	export void LogVR4300Instruction(u64 pc, u64 instr_code, u32 instr_phys_addr, unsigned p_cycle);
}