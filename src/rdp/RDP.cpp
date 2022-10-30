module RDP;

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
			return false; // TODO

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
		if constexpr (cmd_loc == CommandLocation::DMEM) {
			return RSP::RspReadCommandByteswapped(addr);
		}
		else {
			return RDRAM::RspReadCommandByteswapped(addr);
		}
	}


	template<CommandLocation cmd_loc>
	void LoadExecuteCommands()
	{
		dp.status.pipe_busy = dp.status.start_gclk = dp.status.freeze = true;

		u32 current = dp.current;
		u32 end = dp.end;
		if (end <= current) {
			return;
		}
		u32 num_dwords = (end - current) / 8;
		if (num_queued_dwords + num_dwords >= cmd_buffer.size() / 2) {
			return;
		}

		do {
			u64 dword = LoadCommandDword<cmd_loc>(current);
			current += 8;
			std::memcpy(cmd_buffer.data() + num_queued_dwords * 2, &dword, 8);
			++num_queued_dwords;
		} while (--num_dwords > 0);

		while (cmd_buffer_dword_idx < num_queued_dwords) {
			u32 cmd = cmd_buffer[cmd_buffer_dword_idx * 2];
			u32 opcode = cmd >> 24 & 0x3F;
			static constexpr std::array cmd_lens = {
				1, 1, 1, 1, 1, 1, 1, 1, 4, 6,12,14,12,14,20,22,
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			};
			u32 cmd_len = cmd_lens[opcode];
			if (cmd_buffer_dword_idx + cmd_len > num_queued_dwords) {
				/* partial command; keep data around for next processing call */
				dp.start = dp.current = dp.end;
				return;
			}
			if (opcode >= 8) {
				implementation->EnqueueCommand(cmd_len * 2, cmd_buffer.data() + cmd_buffer_dword_idx * 2);
			}
			if (opcode == 0x29) { /* full sync command */
				implementation->OnFullSync();
				dp.status.pipe_busy = dp.status.start_gclk = false;
				MI::SetInterruptFlag(MI::InterruptType::DP);
				
			}
			cmd_buffer_dword_idx += cmd_len;
		}

		cmd_buffer_dword_idx = num_queued_dwords = 0;
		dp.status.pipe_busy = dp.status.start_gclk = dp.status.freeze = 0;
		dp.current = dp.end;
	}


	s32 ReadReg(u32 addr)
	{
		/* TODO: RCP will ignore the requested access size and will just put the requested 32-bit word on the bus.
		Luckily, this is the correct behavior for 8-bit and 16-bit accesses (as explained above), so the VR4300 will
		be able to extract the correct portion. 64-bit reads instead will completely freeze the VR4300
		(and thus the whole console), because it will stall waiting for the second word to appear on the bus that the RCP will never put. */
		auto reg_index = addr >> 2 & 7;
		s32 ret;
		std::memcpy(&ret, (s32*)(&dp) + reg_index, 4);
		return ret;
	}


	void WriteReg(u32 addr, s32 data)
	{
		auto ProcessCommands = [&] {
			dp.status.dmem_dma
				? LoadExecuteCommands<CommandLocation::RDRAM>()
				: LoadExecuteCommands<CommandLocation::DMEM>();
		};

		auto reg_index = addr >> 2 & 7;

		enum Register {
			StartReg, EndReg, CurrentReg, StatusReg, ClockReg, BufBusyReg, PipeBusyReg, TmemReg
		};

		switch (reg_index) {
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
				dp.status.dmem_dma = 0;
			}
			else if (data & 2) {
				dp.status.dmem_dma = 1;
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