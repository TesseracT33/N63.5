export module Logging;

import NumericalTypes;

import <format>;
import <fstream>;
import <string>;
import <string_view>;
import <type_traits>;

namespace Logging
{
	export
	{
		void LogMemoryRead(u32 physical_address, auto memory_value);
		void LogMemoryWrite(u32 physical_address, auto memory_value);
		void LogIORead(u32 physical_address, auto memory_value, std::string_view location);
		void LogIOWrite(u32 physical_address, auto memory_value, std::string_view location);
		void LogVR4300Instruction(u64 pc, const std::string& instr_output, u32 instr_phys_addr);
		void LogRSPInstruction(u32 pc, const std::string& instr_output);
		void LogDMA(const std::string& output);
		void LogException(std::string_view exception);
	}

	std::ofstream instr_logging_ofs{ "F:\\n64.txt" };
}