export module RDP;

import RDPImplementation;
import Util;

import <SDL.h>;

import <array>;
import <cassert>;
import <cstring>;
import <memory>;

namespace RDP
{
	export
	{
		enum class Implementation {
			None, ParallelRDP
		};

		bool Initialize(Implementation rdp_implementation);
		s32 ReadWord(u32 addr);
		void WriteWord(u32 addr, s32 data);

		std::unique_ptr<RDPImplementation> implementation;
	}

	enum class CommandLocation {
		DMEM, RDRAM
	};

	struct
	{
		u32 start, end, current;
		struct
		{
			u32 dmem_dma_status : 1;
			u32 freeze_status : 1;
			u32 flush_status : 1;
			u32 start_gclk : 1;
			u32 tmem_busy : 1;
			u32 pipe_busy : 1;
			u32 command_busy : 1;
			u32 command_buffer_busy : 1;
			u32 dma_busy : 1;
			u32 end_valid : 1;
			u32 start_valid : 1;
			u32 : 21;
		} status;
		u32 clock, bufbusy, pipebusy, tmem;
	} dp;

	template<CommandLocation> u64 LoadCommandDword(u32 addr);
	template<CommandLocation> void LoadExecuteCommands();

	u32 cmd_buffer_dword_idx;
	u32 num_queued_dwords;
	std::array<u32, 0x100000> cmd_buffer;
}