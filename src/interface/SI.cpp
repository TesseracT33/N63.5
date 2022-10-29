module SI;

import DMA;
import Memory;
import MI;

namespace SI
{
	void ClearStatusFlag(StatusFlag status_flag)
	{
		si.status &= ~std::to_underlying(status_flag);
	}


	void Initialize()
	{

	}


	s32 ReadReg(u32 addr)
	{
		u32 offset = (addr >> 2) & 7;
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
		u32 offset = (addr >> 2) & 7;

		enum RegOffset {
			DramAddr = 0, AddrRd64B = 1, AddrWr4B = 2,
			AddrWr64B = 4, AddrRd4B = 5, Status = 6
		};

		switch (offset)
		{
		case RegOffset::DramAddr:
			si.dram_addr = data;
			break;

		case RegOffset::AddrRd4B:
			si.pif_addr_rd4b = data;
			DMA::Init<DMA::Type::SI, DMA::Location::PIF, DMA::Location::RDRAM>(4, si.pif_addr_rd4b, si.dram_addr);
			break;

		case RegOffset::AddrRd64B:
			si.pif_addr_rd64b = data;
			DMA::Init<DMA::Type::SI, DMA::Location::PIF, DMA::Location::RDRAM>(64, si.pif_addr_rd64b, si.dram_addr);
			break;

		case RegOffset::AddrWr4B:
			si.pif_addr_wr4b = data;
			DMA::Init<DMA::Type::SI, DMA::Location::RDRAM, DMA::Location::PIF>(4, si.dram_addr, si.pif_addr_wr4b);
			break;

		case RegOffset::AddrWr64B:
			si.pif_addr_wr64b = data;
			DMA::Init<DMA::Type::SI, DMA::Location::RDRAM, DMA::Location::PIF>(64, si.dram_addr, si.pif_addr_wr64b);
			break;

		case RegOffset::Status:
			/* Writing any value to si.STATUS clears bit 12 (SI Interrupt flag), not only here,
			   but also in the RCP Interrupt Cause register and in MI. */
			si.status &= ~0x1000;
			MI::ClearInterruptFlag(MI::InterruptType::SI);
			// TODO: RCP flag
			break;

		default: 
			break;
		}
	}
}