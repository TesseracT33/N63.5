export module Logging;

import Util;

import <concepts>;
import <format>;
import <fstream>;
import <string>;
import <string_view>;


bool log_enabled;
std::ofstream log_;
std::string prev_output;
u64 repeat_counter;


export void Log(const std::string& output)
{
	if (!log_enabled) return;
	if (output == prev_output) {
		++repeat_counter;
	}
	else {
		if (repeat_counter > 0) {
			log_ << "<<< Repeated " << repeat_counter << " time(s). >>>\n";
			repeat_counter = 0;
		}
		prev_output = output;
		log_ << output << '\n';
	}
}


export void LogCpuRead(u32 phys_addr, std::integral auto value)
{
	if (!log_enabled) return;
	Log(std::format("CPU READ; ${:0X} from ${:08X}", MakeUnsigned(value), phys_addr));
}


export void LogCpuInstruction(u32 instr_phys_addr, const auto& instr_output)
{
	if (!log_enabled) return;
	Log(std::format("CPU; ${:08X}  {}", instr_phys_addr, instr_output));
}


export void LogCpuWrite(u32 phys_addr, std::integral auto value)
{
	if (!log_enabled) return;
	Log(std::format("CPU WRITE; ${:0X} to ${:08X}", MakeUnsigned(value), phys_addr));
}


export void LogDma(const auto& output)
{
	if (!log_enabled) return;
	Log(std::format("INIT DMA; {}", output));
}


export void LogException(const auto& exception)
{
	if (!log_enabled) return;
	Log(std::format("EXCEPTION; {}", exception));
}


export void LogIoRead(std::string_view loc, std::string_view reg, std::integral auto value)
{
	if (!log_enabled) return;
	Log(std::format("{} IO: {} => ${:08X}", loc, reg, MakeUnsigned(value)));
}


export void LogIoWrite(std::string_view loc, std::string_view reg, std::integral auto value)
{
	if (!log_enabled) return;
	Log(std::format("{} IO: {} <= ${:08X}", loc, reg, MakeUnsigned(value)));
}


export void LogRspRead(u32 dmem_addr, std::integral auto value)
{
	if (!log_enabled) return;
	Log(std::format("RSP READ; ${:0X} from DMEM ${:03X}", MakeUnsigned(value), dmem_addr));
}


export void LogRspInstruction(u32 pc, const auto& instr_output)
{
	if (!log_enabled) return;
	Log(std::format("RSP; ${:03X}  {}", pc, instr_output));
}


export void LogRspWrite(u32 dmem_addr, std::integral auto value)
{
	if (!log_enabled) return;
	Log(std::format("RSP WRITE; ${:0X} to DMEM ${:03X}", MakeUnsigned(value), dmem_addr));
}


export void SetLogPath(const auto& log_path)
{
	if (log_.is_open()) {
		log_.close();
	}
	log_.open(log_path);
	log_enabled = log_.is_open();
}