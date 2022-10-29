module Logging;


import Util;


namespace Logging
{
	void LogMemoryRead(u32 physical_address, auto memory_value)
	{
		if (!enabled) return;
		std::string output = std::format(
			"READ; ${:0X} from ${:08X}\n", MakeUnsigned(memory_value), physical_address);
		log << output;
	}


	void LogMemoryWrite(u32 physical_address, auto memory_value)
	{
		if (!enabled) return;
		std::string output = std::format(
			"WRITE; ${:0X} to ${:08X}\n", MakeUnsigned(memory_value), physical_address);
		log << output;
	}


	void LogIORead(u32 physical_address, auto memory_value, const std::string_view location)
	{
		if (!enabled) return;
		std::string output = std::format(
			"{} READ; ${:X} from ${:08X}\n", location, MakeUnsigned(memory_value), physical_address);
		log << output;
	}


	void LogIOWrite(u32 physical_address, auto memory_value, const std::string_view location)
	{
		if (!enabled) return;
		std::string output = std::format(
			"{} WRITE; ${:X} to ${:08X}\n", location, MakeUnsigned(memory_value), physical_address);
		log << output;
	}


	void LogVR4300Instruction(u64 pc, const std::string& instr_output, u32 instr_phys_addr)
	{
		if (!enabled) return;
		std::string output = std::format(
			"VR4300; ${:016X} \t origin: ${:08X} \t {}\n", pc, instr_phys_addr, instr_output);
		log << output;
	}


	void LogRSPInstruction(u32 pc, const std::string& instr_output)
	{
		if (!enabled) return;
		std::string output = std::format("RSP; ${:03X} \t {}\n", pc, instr_output);
		log << output;
	}


	void LogDMA(const std::string& output)
	{
		if (!enabled) return;
		log << "STARTED DMA; " << output << std::endl;
	}


	void LogException(const std::string_view exception)
	{
		if (!enabled) return;
		log << "EXCEPTION; " << exception << std::endl;
	}


	void LogMisc(const std::string& output)
	{
		if (!enabled) return;
		log << output << std::endl;
	}


	void SetPath(const std::string& log_path)
	{
		if (log.is_open()) {
			log.close();
		}
		log.open(log_path);
		enabled = log.is_open();
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

	template void LogIORead(u32, u8, std::string_view);
	template void LogIORead(u32, s8, std::string_view);
	template void LogIORead(u32, u16, std::string_view);
	template void LogIORead(u32, s16, std::string_view);
	template void LogIORead(u32, u32, std::string_view);
	template void LogIORead(u32, s32, std::string_view);
	template void LogIORead(u32, u64, std::string_view);
	template void LogIORead(u32, s64, std::string_view);
	template void LogIOWrite(u32, u8, std::string_view);
	template void LogIOWrite(u32, s8, std::string_view);
	template void LogIOWrite(u32, u16, std::string_view);
	template void LogIOWrite(u32, s16, std::string_view);
	template void LogIOWrite(u32, u32, std::string_view);
	template void LogIOWrite(u32, s32, std::string_view);
	template void LogIOWrite(u32, u64, std::string_view);
	template void LogIOWrite(u32, s64, std::string_view);
}