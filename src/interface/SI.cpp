module SI;

import DMA;
import Memory;
import MI;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace SI
{
	void ClearStatusFlag(StatusFlag status_flag)
	{
		si.status &= ~std::to_underlying(status_flag);
	}


	void Initialize()
	{

	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		u32 offset = (addr >> 2) & 7;
		s32 ret;
		std::memcpy(&ret, (s32*)(&si) + offset, 4);
		return Int(ret);
	}


	void SetStatusFlag(StatusFlag status_flag)
	{
		si.status |= std::to_underlying(status_flag);
	}


	template<size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		/* TODO: for now, only allow word-aligned writes. Force 'data' to be a 32-bit integer. */
		u32 offset = (addr >> 2) & 7;
		auto word = static_cast<s32>(data);

		enum RegOffset {
			DramAddr = 0, AddrRd64B = 1, AddrWr4B = 2,
			AddrWr64B = 4, AddrRd4B = 5, Status = 6
		};

		switch (offset)
		{
		case RegOffset::DramAddr:
			si.dram_addr = word;
			break;

		case RegOffset::AddrRd4B:
			si.pif_addr_rd4b = word;
			DMA::Init<DMA::Type::SI, DMA::Location::PIF, DMA::Location::RDRAM>(4, si.pif_addr_rd4b, si.dram_addr);
			break;

		case RegOffset::AddrRd64B:
			si.pif_addr_rd64b = word;
			DMA::Init<DMA::Type::SI, DMA::Location::PIF, DMA::Location::RDRAM>(64, si.pif_addr_rd64b, si.dram_addr);
			break;

		case RegOffset::AddrWr4B:
			si.pif_addr_wr4b = word;
			DMA::Init<DMA::Type::SI, DMA::Location::RDRAM, DMA::Location::PIF>(4, si.dram_addr, si.pif_addr_wr4b);
			break;

		case RegOffset::AddrWr64B:
			si.pif_addr_wr64b = word;
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


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, u32)
}