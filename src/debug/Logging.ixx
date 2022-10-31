export module Logging;

import Util;

import <concepts>;
import <format>;
import <fstream>;
import <string>;


bool log_enabled;
std::ofstream log_;


export void Log(const std::string& output)
{
	if (!log_enabled) return;
	log_ << output << '\n';
}


export void Log(const char* output)
{
	if (!log_enabled) return;
	log_ << output << '\n';
}


export void LogCpuRead(u32 phys_addr, std::integral auto value)
{
	if (!log_enabled) return;
	std::string output = std::format("CPU READ; ${:0X} from ${:08X}\n", MakeUnsigned(value), phys_addr);
	log_ << output;
}


export void LogCpuInstruction(u32 instr_phys_addr, const std::string& instr_output)
{
	if (!log_enabled) return;
	std::string output = std::format("CPU; ${:08X}  {}\n", instr_phys_addr, instr_output);
	log_ << output;
}


export void LogCpuWrite(u32 phys_addr, std::integral auto value)
{
	if (!log_enabled) return;
	std::string output = std::format("CPU WRITE; ${:0X} to ${:08X}\n", MakeUnsigned(value), phys_addr);
	log_ << output;
}


export void LogDma(const auto& output)
{
	if (!log_enabled) return;
	log_ << "INIT DMA; " << output << '\n';
}


export void LogException(const auto& exception)
{
	if (!log_enabled) return;
	log_ << "EXCEPTION; " << exception << '\n';
}


export void LogIoRead(u32 phys_addr, std::integral auto value, const auto& io_loc)
{
	if (!log_enabled) return;
	std::string output = std::format("{} READ; ${:X} from ${:08X}\n", io_loc, MakeUnsigned(value), phys_addr);
	log_ << output;
}


export void LogIoWrite(u32 phys_addr, std::integral auto value, const auto& io_loc)
{
	if (!log_enabled) return;
	std::string output = std::format("{} WRITE; ${:X} to ${:08X}\n", io_loc, MakeUnsigned(value), phys_addr);
	log_ << output;
}


export void LogRspRead(u32 dmem_addr, std::integral auto value)
{
	if (!log_enabled) return;
	std::string output = std::format("RSP READ; ${:0X} from DMEM ${:03X}\n", MakeUnsigned(value), dmem_addr);
	log_ << output;
}


export void LogRspInstruction(u32 pc, const std::string& instr_output)
{
	if (!log_enabled) return;
	std::string output = std::format("RSP; ${:03X}  {}\n", pc, instr_output);
	log_ << output;
}


export void LogRspWrite(u32 dmem_addr, std::integral auto value)
{
	if (!log_enabled) return;
	std::string output = std::format("RSP WRITE; ${:0X} to DMEM ${:03X}\n", MakeUnsigned(value), dmem_addr);
	log_ << output;
}


export void SetLogPath(const std::string& log_path)
{
	if (log_.is_open()) {
		log_.close();
	}
	log_.open(log_path);
	log_enabled = log_.is_open();
}