module SI;

import BuildOptions;
import Logging;
import MI;
import PIF;
import RDRAM;
import Scheduler;

namespace SI
{
	void ClearStatusFlag(StatusFlag status_flag)
	{
		si.status &= ~std::to_underlying(status_flag);
	}


	template<DmaType type>
	void InitDma()
	{
		SetStatusFlag(StatusFlag::DmaBusy);
		u32 pif_addr;
		size_t bytes_until_pif_end;
		if constexpr (type == DmaType::PifToRdram) {
			pif_addr = si.pif_addr_rd64b;
			pif_addr_reg_last_dma = &si.pif_addr_rd64b;
		}
		else { /* RDRAM to PIF */
			pif_addr = si.pif_addr_wr64b;
			pif_addr_reg_last_dma = &si.pif_addr_wr64b;
		}
		bytes_until_pif_end = PIF::GetNumberOfBytesUntilMemoryEnd(pif_addr);
		u8* rdram_ptr = RDRAM::GetPointerToMemory(si.dram_addr);
		size_t bytes_until_rdram_end = RDRAM::GetNumberOfBytesUntilMemoryEnd(si.dram_addr);
		static constexpr size_t max_dma_len = 64;
		dma_len = std::min(max_dma_len, std::min(bytes_until_rdram_end, bytes_until_pif_end));
		if constexpr (type == DmaType::PifToRdram) {
			u8* pif_ptr = PIF::GetPointerToMemory(pif_addr);
			std::memcpy(rdram_ptr, pif_ptr, dma_len);
			if constexpr (log_dma) {
				LogDma(std::format("From PIF ${:X} to RDRAM ${:X}: ${:X} bytes",
					pif_addr, si.dram_addr, dma_len));
			}
		}
		else { /* RDRAM to PIF */
			size_t num_bytes_in_rom_area = PIF::GetNumberOfBytesUntilRamStart(pif_addr);
			if (num_bytes_in_rom_area < dma_len) {
				pif_addr += num_bytes_in_rom_area;
				rdram_ptr += num_bytes_in_rom_area;
				dma_len -= num_bytes_in_rom_area;
				for (size_t i = 0; i < dma_len; i += 4) {
					s32 val;
					std::memcpy(&val, rdram_ptr, 4);
					PIF::WriteMemory<4>(pif_addr, val);
					pif_addr += 4;
					rdram_ptr += 4;
				}
				if constexpr (log_dma) {
					LogDma(std::format("From RDRAM ${:X} to PIF ${:X}: ${:X} bytes",
						si.dram_addr, pif_addr, dma_len - num_bytes_in_rom_area));
				}
			}
			else {
				LogDma(std::format("Attempted from RDRAM ${:X} to PIF ${:X}, but the target PIF memory area was entirely in the ROM region",
					si.dram_addr, pif_addr));
				OnDmaFinish();
				return;
			}
		}

		static constexpr auto cycles_per_byte_dma = 18;
		auto cycles_until_finish = dma_len * cycles_per_byte_dma;
		Scheduler::AddEvent(Scheduler::EventType::SiDmaFinish, cycles_until_finish, OnDmaFinish);
	}


	void Initialize()
	{
		std::memset(&si, 0, sizeof(si));
	}


	void OnDmaFinish()
	{
		SetStatusFlag(StatusFlag::Interrupt);
		ClearStatusFlag(StatusFlag::DmaBusy);
		MI::SetInterruptFlag(MI::InterruptType::SI);
		si.dram_addr = (si.dram_addr + dma_len) & 0xFF'FFFF;
		*pif_addr_reg_last_dma = (*pif_addr_reg_last_dma + dma_len) & 0x7FC;
	}


	s32 ReadReg(u32 addr)
	{
		static_assert(sizeof(si) >> 2 == 8);
		u32 offset = addr >> 2 & 7;
		s32 ret;
		std::memcpy(&ret, (s32*)(&si) + offset, 4);
		if constexpr (log_io_si) {
			LogIoRead("SI", RegOffsetToStr(offset), ret);
		}
		return ret;
	}


	constexpr std::string_view RegOffsetToStr(u32 reg_offset)
	{
		switch (reg_offset) {
		case DramAddr: return "SI_DRAM_ADDR";
		case AddrRd64B: return "SI_ADDR_RD64B";
		case AddrWr4B: return "SI_ADDR_WR4B";
		case AddrWr64B: return "SI_ADDR_WR64B";
		case AddrRd4B: return "SI_ADDR_RD4B";
		case Status: return "SI_STATUS";
		default: return "UNKNOWN";
		}
	}


	void SetStatusFlag(StatusFlag status_flag)
	{
		si.status |= std::to_underlying(status_flag);
	}


	void WriteReg(u32 addr, s32 data)
	{
		static_assert(sizeof(si) >> 2 == 8);
		u32 offset = addr >> 2 & 7;
		if constexpr (log_io_ai) {
			LogIoWrite("SI", RegOffsetToStr(offset), data);
		}

		switch (offset) {
		case Register::DramAddr:
			si.dram_addr = data & 0xFF'FFF8;
			break;

		case Register::AddrRd64B:
			si.pif_addr_rd64b = data & ~1;
			InitDma<DmaType::PifToRdram>();
			break;

		case Register::AddrWr4B:
			si.pif_addr_wr4b = data;
			/* TODO */
			Log("Tried to start SI WR4B DMA, which is currently unimplemented.");
			break;

		case Register::AddrWr64B:
			si.pif_addr_wr64b = data & ~1;
			InitDma<DmaType::RdramToPif>();
			break;

		case Register::AddrRd4B:
			si.pif_addr_rd4b = data;
			/* TODO */
			Log("Tried to start SI RD4B DMA, which is currently unimplemented.");
			break;

		case Register::Status:
			/* Writing any value to si.STATUS clears bit 12 (SI Interrupt flag), not only here,
			   but also in the RCP Interrupt Cause register and in MI. */
			ClearStatusFlag(StatusFlag::Interrupt);
			MI::ClearInterruptFlag(MI::InterruptType::SI);
			// TODO: RCP flag
			break;

		default:
			Log(std::format("Unexpected write made to SI register at address ${:08X}", addr));
		}
	}
}