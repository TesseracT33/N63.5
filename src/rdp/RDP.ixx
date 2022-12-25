export module RDP;

import RDPImplementation;
import Util;

import <array>;
import <cassert>;
import <cstring>;
import <memory>;
import <string_view>;

namespace RDP
{
	export
	{
		enum class Implementation {
			None, ParallelRDP
		};

		bool Initialize(Implementation rdp_implementation);
		s32 ReadReg(u32 addr);
		void WriteReg(u32 addr, s32 data);

		std::unique_ptr<RDPImplementation> implementation;
	}

	enum class CommandLocation {
		DMEM, RDRAM
	};

	enum Register {
		StartReg, EndReg, CurrentReg, StatusReg, ClockReg, BufBusyReg, PipeBusyReg, TmemReg
	};

	struct {
		u32 start, end, current;
		struct {
			u32 cmd_source : 1;
			u32 freeze : 1;
			u32 flush : 1;
			u32 start_gclk : 1;
			u32 tmem_busy : 1;
			u32 pipe_busy : 1;
			u32 command_busy : 1;
			u32 ready : 1;
			u32 dma_busy : 1;
			u32 end_valid : 1;
			u32 start_valid : 1;
			u32 : 21;
		} status;
		u32 clock, bufbusy, pipebusy, tmem;
	} dp;

	template<CommandLocation> u64 LoadCommandDword(u32 addr);
	template<CommandLocation> void LoadExecuteCommands();
	constexpr std::string_view RegOffsetToStr(u32 reg_offset);

	u32 queue_word_offset;
	u32 num_queued_words;
	std::array<u32, 0x100000> cmd_buffer;
	constexpr u32 cmd_buffer_word_capacity = cmd_buffer.size();
}