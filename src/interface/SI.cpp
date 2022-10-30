module SI;

import DebugOptions;
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
		ClearStatusFlag(StatusFlag::Interrupt); /* TODO: should we clear? (also MI flag) */
		MI::ClearInterruptFlag(MI::InterruptType::SI); 
		/* pif address is aligned to four bytes */
		u32 pif_addr;
		size_t bytes_until_pif_end;
		if constexpr (type == DmaType::PifToRdram) {
			pif_addr = si.pif_addr_rd64b;
			pif_addr_reg_last_dma = &si.pif_addr_rd64b;
			bytes_until_pif_end = PIF::GetNumberOfBytesUntilMemoryEnd(pif_addr);
		}
		else { /* RDRAM to PIF */
			pif_addr = si.pif_addr_wr64b;
			pif_addr_reg_last_dma = &si.pif_addr_wr64b;
			bytes_until_pif_end = PIF::GetNumberOfBytesUntilRamStart(pif_addr);
		}
		u8* rdram_ptr = RDRAM::GetPointerToMemory(si.dram_addr);
		u8* pif_ptr = PIF::GetPointerToMemory(pif_addr);
		size_t bytes_until_rdram_end = RDRAM::GetNumberOfBytesUntilMemoryEnd(si.dram_addr);
		dma_len = std::min(size_t(64), std::min(bytes_until_rdram_end, bytes_until_pif_end));
		if constexpr (type == DmaType::PifToRdram) {
			std::memcpy(rdram_ptr, pif_ptr, dma_len);
			if constexpr (log_dma) {
				Logging::LogDMA(std::format("From PIF ${:X} to RDRAM ${:X}: ${:X} bytes",
					pif_addr, si.dram_addr, dma_len));
			}
		}
		else { /* RDRAM to PIF */
			std::memcpy(pif_ptr, rdram_ptr, dma_len);
			Logging::LogDMA(std::format("From RDRAM ${:X} to PIF ${:X}: ${:X} bytes",
				si.dram_addr, pif_addr, dma_len));
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
		u32 offset = addr >> 2 & 7;
		s32 ret;
		std::memcpy(&ret, (s32*)(&si) + offset, 4);
		return ret;
	}


	void SetStatusFlag(StatusFlag status_flag)
	{
		si.status |= std::to_underlying(status_flag);
	}


	void WriteReg(u32 addr, s32 data)
	{
		u32 offset = addr >> 2 & 7;

		enum RegOffset {
			DramAddr = 0, AddrRd64B = 1, AddrWr4B = 2,
			AddrWr64B = 4, AddrRd4B = 5, Status = 6
		};

		switch (offset) {
		case RegOffset::DramAddr:
			si.dram_addr = data & 0xFF'FFFF;
			break;

		case RegOffset::AddrRd4B:
			si.pif_addr_rd4b = data;
			/* TODO */
			Logging::LogMisc("Tried to start SI RD4B DMA, which is currently unimplemented.");
			break;

		case RegOffset::AddrRd64B:
			si.pif_addr_rd64b = data & 0x7FC;
			InitDma<DmaType::PifToRdram>();
			break;

		case RegOffset::AddrWr4B:
			si.pif_addr_wr4b = data;
			/* TODO */
			Logging::LogMisc("Tried to start SI WR4B DMA, which is currently unimplemented.");
			break;

		case RegOffset::AddrWr64B:
			si.pif_addr_wr64b = data & 0x7FC;
			InitDma<DmaType::RdramToPif>();
			break;

		case RegOffset::Status:
			/* Writing any value to si.STATUS clears bit 12 (SI Interrupt flag), not only here,
			   but also in the RCP Interrupt Cause register and in MI. */
			ClearStatusFlag(StatusFlag::Interrupt);
			MI::ClearInterruptFlag(MI::InterruptType::SI);
			// TODO: RCP flag
			break;

		default: 
			break;
		}
	}
}