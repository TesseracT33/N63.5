module SI;

import DMA;
import Memory;
import MI;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace SI
{
	void Initialize()
	{

	}


	template<StatusFlag status_flag>
	void SetStatusFlag()
	{
		si.status |= std::to_underlying(status_flag);
	}


	template<StatusFlag status_flag>
	void ClearStatusFlag()
	{
		si.status &= ~std::to_underlying(status_flag);
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		const u32 offset = (addr >> 2) & 7;
		s32 ret;
		std::memcpy(&ret, (s32*)(&si) + offset, 4);
		return Int(ret);
	}


	template<std::size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		/* TODO: for now, only allow word-aligned writes. Force 'data' to be a 32-bit integer. */
		const u32 offset = (addr >> 2) & 7;
		const auto word = static_cast<s32>(data);

		static constexpr u32 offset_dram_addr = 0;
		static constexpr u32 offset_addr_rd64b = 1;
		static constexpr u32 offset_addr_rd4b = 2;
		static constexpr u32 offset_addr_wr64b = 4;
		static constexpr u32 offset_addr_wr4b = 5;
		static constexpr u32 offset_status = 6;

		switch (offset)
		{
		case offset_dram_addr:
			si.dram_addr = word;
			break;

		case offset_addr_rd4b:
			si.pif_addr_rd4b = word;
			DMA::Init<DMA::Type::SI, DMA::Location::PIF, DMA::Location::RDRAM>(4, si.pif_addr_rd4b, si.dram_addr);
			break;

		case offset_addr_rd64b:
			si.pif_addr_rd64b = word;
			DMA::Init<DMA::Type::SI, DMA::Location::PIF, DMA::Location::RDRAM>(64, si.pif_addr_rd64b, si.dram_addr);
			break;

		case offset_addr_wr4b:
			si.pif_addr_wr4b = word;
			DMA::Init<DMA::Type::SI, DMA::Location::RDRAM, DMA::Location::PIF>(4, si.dram_addr, si.pif_addr_wr4b);
			break;

		case offset_addr_wr64b:
			si.pif_addr_wr64b = word;
			DMA::Init<DMA::Type::SI, DMA::Location::RDRAM, DMA::Location::PIF>(64, si.dram_addr, si.pif_addr_wr64b);
			break;

		case offset_status:
			/* Writing any value to si.STATUS clears bit 12 (SI Interrupt flag), not only here,
			   but also in the RCP Interrupt Cause register and in MI. */
			si.status &= ~0x1000;
			MI::ClearInterruptFlag<MI::InterruptType::SI>();
			// TODO: RCP flag
			break;

		default: 
			break;
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, u32)


	template void ClearStatusFlag<StatusFlag::DMA_BUSY>();
	template void ClearStatusFlag<StatusFlag::IO_BUSY>();
	template void ClearStatusFlag<StatusFlag::READ_PENDING>();
	template void ClearStatusFlag<StatusFlag::DMA_ERROR>();
	template void ClearStatusFlag<StatusFlag::INTERRUPT>();
	template void SetStatusFlag<StatusFlag::DMA_BUSY>();
	template void SetStatusFlag<StatusFlag::IO_BUSY>();
	template void SetStatusFlag<StatusFlag::READ_PENDING>();
	template void SetStatusFlag<StatusFlag::DMA_ERROR>();
	template void SetStatusFlag<StatusFlag::INTERRUPT>();
}