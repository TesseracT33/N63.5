module PI;

import DMA;
import Memory;
import MI;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace PI
{
	void Initialize()
	{

	}


	template<StatusFlag status_flag>
	void SetStatusFlag()
	{
		pi.status |= std::to_underlying(status_flag);
	}


	template<StatusFlag status_flag>
	void ClearStatusFlag()
	{
		pi.status &= ~std::to_underlying(status_flag);
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		const u32 offset = (addr >> 2) & 0xF;
		s32 ret;
		std::memcpy(&ret, (s32*)(&pi) + offset, 4);
		return Int(ret);
	}


	template<std::size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		/* TODO: for now, only allow word-aligned writes. Force 'data' to be a 32-bit integer. */
		const auto word = static_cast<s32>(data);
		const u32 offset = (addr >> 2) & 0xF;

		static constexpr u32 offset_dram_addr = 0;
		static constexpr u32 offset_cart_addr = 1;
		static constexpr u32 offset_rd_len = 2;
		static constexpr u32 offset_wr_len = 3;
		static constexpr u32 offset_status = 4;
		static constexpr u32 offset_bsd_dom1_lat = 5;
		static constexpr u32 offset_bsd_dom1_pwd = 6;
		static constexpr u32 offset_bsd_dom1_pgs = 7;
		static constexpr u32 offset_bsd_dom1_rls = 8;
		static constexpr u32 offset_bsd_dom2_lat = 9;
		static constexpr u32 offset_bsd_dom2_pwd = 10;
		static constexpr u32 offset_bsd_dom2_pgs = 11;
		static constexpr u32 offset_bsd_dom2_rls = 12;

		switch (offset) 
		{
		case offset_dram_addr:
			pi.dram_addr = word;
			break;

		case offset_cart_addr:
			pi.cart_addr = word;
			break;

		case offset_rd_len:
			pi.rd_len = word;
			/* Initialize DMA transfer */
			DMA::Init<DMA::Type::PI, DMA::Location::RDRAM, DMA::Location::Cartridge>(pi.rd_len + 1, pi.dram_addr, pi.cart_addr);
			SetStatusFlag<StatusFlag::DMA_BUSY>();
			ClearStatusFlag<StatusFlag::DMA_COMPLETED>();
			MI::ClearInterruptFlag<MI::InterruptType::PI>();
			break;

		case offset_wr_len:
			pi.wr_len = word;
			/* Initialize DMA transfer */
			DMA::Init<DMA::Type::PI, DMA::Location::Cartridge, DMA::Location::RDRAM>(pi.rd_len + 1, pi.cart_addr, pi.dram_addr);
			SetStatusFlag<StatusFlag::DMA_BUSY>();
			ClearStatusFlag<StatusFlag::DMA_COMPLETED>();
			MI::ClearInterruptFlag<MI::InterruptType::PI>();
			break;

		case offset_status:
		{
			constexpr static s32 reset_dma_mask = 0x01;
			constexpr static s32 clear_interrupt_mask = 0x02;
			if (word & reset_dma_mask)
			{
				/* Reset the DMA controller and stop any transfer being done */
				pi.status = 0;
				MI::ClearInterruptFlag<MI::InterruptType::PI>();

			}
			if (word & clear_interrupt_mask)
			{
				/* Clear Interrupt */
				ClearStatusFlag<StatusFlag::DMA_COMPLETED>();
				MI::ClearInterruptFlag<MI::InterruptType::PI>();
			}
			break;
		}

		default: /* TODO */
			break;
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, u32);

	template void ClearStatusFlag<StatusFlag::DMA_BUSY>();
	template void ClearStatusFlag<StatusFlag::IO_BUSY>();
	template void ClearStatusFlag<StatusFlag::DMA_ERROR>();
	template void ClearStatusFlag<StatusFlag::DMA_COMPLETED>();
	template void SetStatusFlag<StatusFlag::DMA_BUSY>();
	template void SetStatusFlag<StatusFlag::IO_BUSY>();
	template void SetStatusFlag<StatusFlag::DMA_ERROR>();
	template void SetStatusFlag<StatusFlag::DMA_COMPLETED>();
}