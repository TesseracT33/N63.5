module PI;

import DMA;
import Memory;
import MI;

namespace PI
{
	void ClearStatusFlag(StatusFlag status_flag)
	{
		pi.status &= ~std::to_underlying(status_flag);
	}


	void Initialize()
	{

	}


	s32 ReadWord(u32 addr)
	{
		const u32 offset = (addr >> 2) & 0xF;
		s32 ret;
		std::memcpy(&ret, (s32*)(&pi) + offset, 4);
		return ret;
	}


	void SetStatusFlag(StatusFlag status_flag)
	{
		pi.status |= std::to_underlying(status_flag);
	}


	void WriteWord(u32 addr, s32 data)
	{
		u32 offset = (addr >> 2) & 0xF;

		enum RegOffset {
			DramAddr, CartAddr, RdLen, WrLen, Status,
			BsdDom1Lat, BsdDom1Pwd, BsdDom1Pgs, BsdDom1Rls,
			BsdDom2Lat, BsdDom2Pwd, BsdDom2Pgs, BsdDom2Rls
		};

		switch (offset) 
		{
		case RegOffset::DramAddr:
			pi.dram_addr = data;
			break;

		case RegOffset::CartAddr:
			pi.cart_addr = data;
			break;

		case RegOffset::RdLen:
			pi.rd_len = data;
			/* Initialize DMA transfer */
			DMA::Init<DMA::Type::PI, DMA::Location::RDRAM, DMA::Location::Cartridge>(pi.rd_len + 1, pi.dram_addr, pi.cart_addr);
			SetStatusFlag(StatusFlag::DmaBusy);
			ClearStatusFlag(StatusFlag::DmaCompleted);
			MI::ClearInterruptFlag(MI::InterruptType::PI);
			break;

		case RegOffset::WrLen:
			pi.wr_len = data;
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
			if (data & reset_dma_mask) {
				/* Reset the DMA controller and stop any transfer being done */
				pi.status = 0;
				MI::ClearInterruptFlag(MI::InterruptType::PI);

			}
			if (data & clear_interrupt_mask) {
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
}