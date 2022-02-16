module Logging;

#include "DebugOptions.h"

std::ofstream instr_logging_ofs{ LOG_PATH };

namespace Logging
{
	void LogMemoryRead(u32 physical_address, auto memory_value)
	{
		const std::string output = std::format(
			"READ;\t${:0X} to ${:08X}\n",
			physical_address, memory_value
		);

		instr_logging_ofs << output;
	}


	void LogMemoryWrite(u32 physical_address, auto memory_value)
	{
		const std::string output = std::format(
			"WRITE;\t${:0X} to ${:08X}\n",
			memory_value, physical_address
		);

		instr_logging_ofs << output;
	}


	void LogVR4300Instruction(u64 pc, u64 instr_code, unsigned p_cycle)
	{
		const std::string output = std::format(
			"INSTR;\tPC: ${:016X} \t instr: ${:08X} \t cycle: {}\n",
			pc, instr_code, p_cycle
		);

		instr_logging_ofs << output;
	}


	template void LogMemoryRead(u32, u8);
	template void LogMemoryRead(u32, s8);
	template void LogMemoryRead(u32, u16);
	template void LogMemoryRead(u32, s16);
	template void LogMemoryRead(u32, u32);
	template void LogMemoryRead(u32, s32);
	template void LogMemoryRead(u32, u64);
	template void LogMemoryRead(u32, s64);
	template void LogMemoryWrite(u32, u8);
	template void LogMemoryWrite(u32, s8);
	template void LogMemoryWrite(u32, u16);
	template void LogMemoryWrite(u32, s16);
	template void LogMemoryWrite(u32, u32);
	template void LogMemoryWrite(u32, s32);
	template void LogMemoryWrite(u32, u64);
	template void LogMemoryWrite(u32, s64);
}