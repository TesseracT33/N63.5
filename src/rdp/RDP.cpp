module RDP;

import DebugOptions;
import FakeRDP;
import Logging;
import MI;
import ParallelRDPWrapper;
import RDRAM;
import RSP;

namespace RDP
{
	bool Initialize(Implementation rdp_implementation)
	{
		std::memset(&dp, 0, sizeof(dp));

		switch (rdp_implementation) {
		case Implementation::None:
			implementation = std::make_unique<FakeRDP>();
			return implementation->Initialize();

		case Implementation::ParallelRDP:
			implementation = std::make_unique<ParallelRDPWrapper>();
			return implementation->Initialize();

		default:
			assert(false);
			return false;
		}
	}


	template<CommandLocation cmd_loc>
	u64 LoadCommandDword(u32 addr)
	{
		if constexpr (cmd_loc == CommandLocation::DMEM) { /* todo: byteswap? */
			return RSP::RdpReadCommandByteswapped(addr);
		}
		else {
			return RDRAM::RdpReadCommandByteswapped(addr);
		}
	}


	template<CommandLocation cmd_loc>
	void LoadExecuteCommands()
	{
		if (dp.status.freeze) {
			return;
		}
		u32 current = dp.current;
		if (dp.end <= current) {
			return;
		}
		dp.status.pipe_busy = dp.status.start_gclk;
		dp.status.ready = false;
		u32 num_dwords = (dp.end - current) / 8;
		if (num_queued_words + 2 * num_dwords >= cmd_buffer_word_capacity) {
			return;
		}

		do {
			u64 dword = LoadCommandDword<cmd_loc>(current); /* TODO: swap words? */
			std::memcpy(&cmd_buffer[num_queued_words], &dword, 8);
			num_queued_words += 2;
			current += 8;
		} while (--num_dwords > 0);

		while (queue_word_offset < num_queued_words) {
			u32 cmd_first_word = cmd_buffer[queue_word_offset];
			u32 opcode = cmd_first_word & 0x3F;
			static constexpr std::array cmd_word_lengths = {
				2, 2, 2, 2, 2, 2, 2, 2, 8,12,24,28,24,28,40,44,
				2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
				2, 2, 2, 2, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
				2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
			};
			u32 cmd_word_len = cmd_word_lengths[opcode];
			if (queue_word_offset + cmd_word_len > num_queued_words) {
				/* partial command; keep data around for next processing call */
				dp.start = dp.current = dp.end;
				return;
			}
			if (opcode >= 8) {
				implementation->EnqueueCommand(cmd_word_len, &cmd_buffer[queue_word_offset]);
			}
			if (opcode == 0x29) { /* full sync command */
				implementation->OnFullSync();
				dp.status.pipe_busy = dp.status.start_gclk = false;
				MI::SetInterruptFlag(MI::InterruptType::DP);

			}
			queue_word_offset += cmd_word_len;
		}

		queue_word_offset = num_queued_words = 0;
		dp.status.ready = true;
		dp.current = dp.end;
	}


	s32 ReadReg(u32 addr)
	{
		/* TODO: RCP will ignore the requested access size and will just put the requested 32-bit word on the bus.
		Luckily, this is the correct behavior for 8-bit and 16-bit accesses (as explained above), so the VR4300 will
		be able to extract the correct portion. 64-bit reads instead will completely freeze the VR4300
		(and thus the whole console), because it will stall waiting for the second word to appear on the bus that the RCP will never put. */
		static_assert(sizeof(dp) >> 2 == 8);
		u32 offset = addr >> 2 & 7;
		s32 ret;
		std::memcpy(&ret, (s32*)(&dp) + offset, 4);
		if constexpr (log_io_rdp) {
			LogIoRead("RDP", RegOffsetToStr(offset), ret);
		}
		return ret;
	}


	constexpr std::string_view RegOffsetToStr(u32 reg_offset)
	{
		switch (reg_offset) {
		case StartReg: return "DPC_START";
		case EndReg: return "DPC_END";
		case CurrentReg: return "DPC_CURRENT";
		case StatusReg: return "DPC_STATUS";
		case ClockReg: return "DPC_CLOCK";
		case BufBusyReg: return "DPC_BUF_BUSY";
		case PipeBusyReg: return "DPC_PIPE_BUSY";
		case TmemReg: return "DPC_TMEM";
		default: std::unreachable();
		}
	}


	void WriteReg(u32 addr, s32 data)
	{
		auto ProcessCommands = [&] {
			dp.status.cmd_source
				? LoadExecuteCommands<CommandLocation::DMEM>()
				: LoadExecuteCommands<CommandLocation::RDRAM>();
		};

		static_assert(sizeof(dp) >> 2 == 8);
		u32 offset = addr >> 2 & 7;
		if constexpr (log_io_rdp) {
			LogIoWrite("RDP", RegOffsetToStr(offset), data);
		}

		switch (offset) {
		case Register::StartReg:
			if (!dp.status.start_valid) {
				dp.start = data & 0xFF'FFF8;
				dp.status.start_valid = true;
			}
			break;

		case Register::EndReg:
			dp.end = data & 0xFF'FFF8;
			if (dp.status.start_valid) {
				dp.current = dp.start;
				dp.status.start_valid = false;
			}
			if (!dp.status.freeze) {
				ProcessCommands();
			}
			break;

		case Register::StatusReg: {
			bool unfrozen = false;
			if (data & 1) {
				dp.status.cmd_source = 0;
			}
			else if (data & 2) {
				dp.status.cmd_source = 1;
			}
			if (data & 4) {
				dp.status.freeze = 0;
				unfrozen = true;
			}
			else if (data & 8) {
				dp.status.freeze = 1;
			}
			if (data & 0x10) {
				dp.status.flush = 0;
			}
			else if (data & 0x20) {
				dp.status.flush = 1;
			}
			if (data & 0x40) {
				dp.tmem = 0;
			}
			if (data & 0x80) {
				dp.pipebusy = 0;
			}
			if (data & 0x100) {
				dp.bufbusy = 0;
			}
			if (data & 0x200) {
				dp.clock = 0;
			}
			if (unfrozen) {
				ProcessCommands();
			}
		} break;
		}
	}
}