module PI;

import DMA;
import Memory;
import MI;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace PI
{
	void ClearStatusFlag(StatusFlag status_flag)
	{
		pi.status &= ~std::to_underlying(status_flag);
	}


	void Initialize()
	{

	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		const u32 offset = (addr >> 2) & 0xF;
		s32 ret;
		std::memcpy(&ret, (s32*)(&pi) + offset, 4);
		return Int(ret);
	}


	void SetStatusFlag(StatusFlag status_flag)
	{
		pi.status |= std::to_underlying(status_flag);
	}


	template<size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		/* TODO: for now, only allow word-aligned writes. Force 'data' to be a 32-bit integer. */
		auto word = static_cast<s32>(data);
		u32 offset = (addr >> 2) & 0xF;

		enum RegOffset {
			DramAddr, CartAddr, RdLen, WrLen, Status,
			BsdDom1Lat, BsdDom1Pwd, BsdDom1Pgs, BsdDom1Rls,
			BsdDom2Lat, BsdDom2Pwd, BsdDom2Pgs, BsdDom2Rls
		};

		switch (offset) 
		{
		case RegOffset::DramAddr:
			pi.dram_addr = word;
			break;

		case RegOffset::CartAddr:
			pi.cart_addr = word;
			break;

		case RegOffset::RdLen:
			pi.rd_len = word;
			/* Initialize DMA transfer */
			DMA::Init<DMA::Type::PI, DMA::Location::RDRAM, DMA::Location::Cartridge>(pi.rd_len + 1, pi.dram_addr, pi.cart_addr);
			SetStatusFlag(StatusFlag::DmaBusy);
			ClearStatusFlag(StatusFlag::DmaCompleted);
			MI::ClearInterruptFlag(MI::InterruptType::PI);
			break;

		case RegOffset::WrLen:
			pi.wr_len = word;
			/* Initialize DMA transfer */
			DMA::Init<DMA::Type::PI, DMA::Location::Cartridge, DMA::Location::RDRAM>(pi.wr_len + 1, pi.cart_addr, pi.dram_addr);
			SetStatusFlag(StatusFlag::DmaBusy);
			ClearStatusFlag(StatusFlag::DmaCompleted);
			MI::ClearInterruptFlag(MI::InterruptType::PI);
			break;

		case RegOffset::Status:
		{
			constexpr static s32 reset_dma_mask = 0x01;
			constexpr static s32 clear_interrupt_mask = 0x02;
			if (word & reset_dma_mask) {
				/* Reset the DMA controller and stop any transfer being done */
				pi.status = 0;
				MI::ClearInterruptFlag(MI::InterruptType::PI);

			}
			if (word & clear_interrupt_mask) {
				/* Clear Interrupt */
				ClearStatusFlag(StatusFlag::DmaCompleted);
				MI::ClearInterruptFlag(MI::InterruptType::PI);
			}
			break;
		}

		default: /* TODO */
			break;
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, u32);
}