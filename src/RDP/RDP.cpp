module RDP;

import RDRAM;
import RSP;

namespace RDP
{
	template<CommandLocation command_location>
	u64 LoadCommandDword(u32 addr)
	{
		dp.current_reg += 8;
		if constexpr (command_location == CommandLocation::DMEM) {
			return RSP::RspReadCommandByteswapped(addr);
		}
		else {
			return RDRAM::RspReadCommandByteswapped(addr);
		}
	}


	template<CommandLocation command_location>
	void LoadDecodeExecuteCommand()
	{
		u64 command = LoadCommandDword<command_location>(dp.start_reg);
		switch (command >> 56 & 0x3F) {
		case 0x24: {
			if (dp.end_reg - dp.start_reg > 8 || dp.end_reg < dp.start_reg) {
				u64 command_upper_dword = LoadCommandDword<command_location>(dp.start_reg + 8);
				TextureRectangle(command, command_upper_dword);
			}
		} break;
		case 0x27: SyncPipe       (command); break;
		case 0x28: SyncTile       (command); break;
		case 0x29: SyncFull       (command); break;
		case 0x2D: SetScissor     (command); break;
		case 0x2F: SetOtherModes  (command); break;
		case 0x31: SyncLoad       (command); break;
		case 0x34: LoadTile       (command); break;
		case 0x35: SetTile        (command); break;
		case 0x36: FillRectangle  (command); break;
		case 0x37: SetFillColor   (command); break;
		case 0x3D: SetTextureImage(command); break;
		case 0x3E: SetZImage      (command); break;
		case 0x3F: SetColorImage  (command); break;
		}
	}


	template<std::integral Int>
	Int ReadReg(u32 addr)
	{
		/* TODO: RCP will ignore the requested access size and will just put the requested 32-bit word on the bus.
		Luckily, this is the correct behavior for 8-bit and 16-bit accesses (as explained above), so the VR4300 will
		be able to extract the correct portion. 64-bit reads instead will completely freeze the VR4300
		(and thus the whole console), because it will stall waiting for the second word to appear on the bus that the RCP will never put. */
		auto offset = addr >> 2 & 7;
		u32 ret;
		std::memcpy(&ret, (u32*)(&dp) + offset, 4);
		return Int(ret);
	}


	template<std::integral Int>
	void WriteReg(u32 addr, Int data)
	{
		auto offset = addr >> 2 & 7;
		auto word = u32(data);

		enum RegOffset {
			StartReg, EndReg, CurrentReg, StatusReg, ClockReg, BufBusyReg, PipeBusyReg, TmemReg
		};

		switch (offset) {
		case StartReg:
			dp.start_reg = word;
			break;

		case EndReg:
			dp.end_reg = word;
			dp.current_reg = dp.start_reg;
			dp.status_reg.dmem_dma_status
				? LoadDecodeExecuteCommand<CommandLocation::DMEM>()
				: LoadDecodeExecuteCommand<CommandLocation::RDRAM>();
			break;

		case CurrentReg:
			dp.current_reg = word;
			break;

		case StatusReg:
			if (word & 1) {
				dp.status_reg.dmem_dma_status = 0;
			}
			else if (word & 2) {
				dp.status_reg.dmem_dma_status = 1;
			}
			if (word & 4) {
				dp.status_reg.freeze_status = 0;
			}
			else if (word & 8) {
				dp.status_reg.freeze_status = 1;
			}
			if (word & 0x10) {
				dp.status_reg.flush_status = 0;
			}
			else if (word & 0x20) {
				dp.status_reg.flush_status = 1;
			}
			if (word & 0x40) {
				dp.tmem_reg = 0;
			}
			if (word & 0x80) {
				dp.pipebusy_reg = 0;
			}
			if (word & 0x100) {
				dp.bufbusy_reg = 0;
			}
			if (word & 0x200) {
				dp.clock_reg = 0;
			}
			break;

		case ClockReg:
			dp.start_reg = word;
			break;

		case BufBusyReg:
			dp.start_reg = word;
			break;

		case PipeBusyReg:
			dp.start_reg = word;
			break;

		case TmemReg:
			dp.start_reg = word;
			break;
		}
	}
}