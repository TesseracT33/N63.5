export module Log;

import BuildOptions;
import Util;

import <concepts>;
import <format>;
import <fstream>;
import <iostream>;
import <string>;
import <string_view>;

namespace Log
{
	export {
		void CpuException(const auto& exception);
		void CpuInstruction(u32 instr_phys_addr, const auto& instr_output);
		void CpuRead(u32 phys_addr, std::integral auto value);
		void CpuWrite(u32 phys_addr, std::integral auto value);
		void Dma(const auto& output);
		void Error(const auto& output);
		void Fatal(const auto& output);
		void Info(const auto& output);
		bool Init();
		void IoRead(std::string_view loc, std::string_view reg, std::integral auto value);
		void IoWrite(std::string_view loc, std::string_view reg, std::integral auto value);
		void RspInstruction(u32 pc, const auto& instr_output);
		void RspRead(u32 dmem_addr, std::integral auto value);
		void RspWrite(u32 dmem_addr, std::integral auto value);
		void Warning(const auto& output);
	}

	void ConsoleOut(const auto& output);
	void FileOut(const auto& output);

	std::ofstream file_log;
	std::string prev_file_output;
	u64 file_output_repeat_counter;
}

void Log::ConsoleOut(const auto& output)
{
	std::cout << output << '\n';
}

void Log::CpuException(const auto& exception)
{
	FileOut(std::format("EXCEPTION; {}", exception));
}

void Log::CpuInstruction(u32 instr_phys_addr, const auto& instr_output)
{
	FileOut(std::format("CPU; ${:08X}  {}", instr_phys_addr, instr_output));
}

void Log::CpuRead(u32 phys_addr, std::integral auto value)
{
	FileOut(std::format("CPU READ; ${:0X} from ${:08X}", MakeUnsigned(value), phys_addr));
}

void Log::CpuWrite(u32 phys_addr, std::integral auto value)
{
	FileOut(std::format("CPU WRITE; ${:0X} to ${:08X}", MakeUnsigned(value), phys_addr));
}

void Log::Dma(const auto& output)
{
	FileOut(std::format("INIT DMA; {}", output));
}

void Log::Error(const auto& output)
{
	std::string shown_output = std::string("[ERROR] ") + output;
	ConsoleOut(shown_output);
	FileOut(shown_output);
}

void Log::Fatal(const auto& output)
{
	std::string shown_output = std::string("[FATAL] ") + output;
	ConsoleOut(shown_output);
	FileOut(shown_output);
}

void Log::FileOut(const auto& output)
{
	if constexpr (enable_file_logging) {
		if (!file_log.is_open()) {
			return;
		}
		if (output == prev_file_output) {
			++file_output_repeat_counter;
		}
		else {
			if (file_output_repeat_counter > 0) {
				file_log << "<<< Repeated " << file_output_repeat_counter << " time(s). >>>\n";
				file_output_repeat_counter = 0;
			}
			prev_file_output = output;
			file_log << output << '\n';
		}
	}
}

void Log::Info(const auto& output)
{
	std::string shown_output = std::string("[INFO] ") + output;
	ConsoleOut(shown_output);
	FileOut(shown_output);
}

bool Log::Init()
{
	if constexpr (enable_file_logging) {
		file_log.open(log_path.data());
		return file_log.is_open();
	}
	else {
		return true;
	}
}

void Log::IoRead(std::string_view loc, std::string_view reg, std::integral auto value)
{
	FileOut(std::format("{} IO: {} => ${:08X}", loc, reg, MakeUnsigned(value)));
}

void Log::IoWrite(std::string_view loc, std::string_view reg, std::integral auto value)
{
	FileOut(std::format("{} IO: {} <= ${:08X}", loc, reg, MakeUnsigned(value)));
}

void Log::RspInstruction(u32 pc, const auto& instr_output)
{
	FileOut(std::format("RSP; ${:03X}  {}", pc, instr_output));
}

void Log::RspRead(u32 dmem_addr, std::integral auto value)
{
	FileOut(std::format("RSP READ; ${:0X} from DMEM ${:03X}", MakeUnsigned(value), dmem_addr));
}

void Log::RspWrite(u32 dmem_addr, std::integral auto value)
{
	FileOut(std::format("RSP WRITE; ${:0X} to DMEM ${:03X}", MakeUnsigned(value), dmem_addr));
}

void Log::Warning(const auto& output)
{
	std::string shown_output = std::string("[WARN] ") + output;
	ConsoleOut(shown_output);
	FileOut(shown_output);
}