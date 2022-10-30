module PI;

import Cartridge;
import DebugOptions;
import Logging;
import MI;
import RDRAM;
import Scheduler;

namespace PI
{
	void ClearStatusFlag(StatusFlag status_flag)
	{
		pi.status &= ~std::to_underlying(status_flag);
	}


	template<DmaType type>
	void InitDma()
	{
		SetStatusFlag(StatusFlag::DmaBusy);
		ClearStatusFlag(StatusFlag::DmaCompleted);
		MI::ClearInterruptFlag(MI::InterruptType::PI);

		u8* rdram_ptr = RDRAM::GetPointerToMemory(pi.dram_addr);
		u8* cart_ptr = Cartridge::GetPointerToRom(pi.cart_addr);
		size_t bytes_until_rdram_end = RDRAM::GetNumberOfBytesUntilMemoryEnd(pi.dram_addr);
		size_t bytes_until_cart_end = Cartridge::GetNumberOfBytesUntilRomEnd(pi.cart_addr);
		size_t dma_len = std::min(bytes_until_rdram_end, bytes_until_cart_end);
		if constexpr (type == DmaType::CartToRdram) {
			dma_len = std::min(dma_len, size_t(pi.wr_len + 1));
			std::memcpy(rdram_ptr, cart_ptr, dma_len);
			if constexpr (log_dma) {
				Logging::LogDMA(std::format("From cart ROM ${:X} to RDRAM ${:X}: ${:X} bytes",
					pi.cart_addr, pi.dram_addr, dma_len));
			}
		}
		else { /* RDRAM to cart */
			dma_len = std::min(dma_len, size_t(pi.rd_len + 1));
			std::memcpy(cart_ptr, rdram_ptr, dma_len);
			Logging::LogDMA(std::format("From RDRAM ${:X} to cart ROM ${:X}: ${:X} bytes",
				pi.dram_addr, pi.cart_addr, dma_len));
		}

		static constexpr auto cycles_per_byte_dma = 18;
		auto cycles_until_finish = dma_len * cycles_per_byte_dma;
		Scheduler::AddEvent(Scheduler::EventType::PiDmaFinish, cycles_until_finish, [] {
			SetStatusFlag(StatusFlag::DmaCompleted);
			ClearStatusFlag(StatusFlag::DmaBusy);
			MI::SetInterruptFlag(MI::InterruptType::PI);
		});
	}


	void Initialize()
	{
		std::memset(&pi, 0, sizeof(pi));
	}


	s32 ReadReg(u32 addr)
	{
		u32 offset = (addr >> 2) & 0xF;
		s32 ret;
		std::memcpy(&ret, (s32*)(&pi) + offset, 4);
		return ret;
	}


	void SetStatusFlag(StatusFlag status_flag)
	{
		pi.status |= std::to_underlying(status_flag);
	}


	void WriteReg(u32 addr, s32 data)
	{
		u32 offset = addr >> 2 & 0xF;

		enum RegOffset {
			DramAddr, CartAddr, RdLen, WrLen, Status,
			BsdDom1Lat, BsdDom1Pwd, BsdDom1Pgs, BsdDom1Rls,
			BsdDom2Lat, BsdDom2Pwd, BsdDom2Pgs, BsdDom2Rls
		};

		switch (offset) {
		case RegOffset::DramAddr:
			pi.dram_addr = data & 0xFF'FFFF;
			break;

		case RegOffset::CartAddr:
			pi.cart_addr = data & 0xFF'FFFF;
			break;

		case RegOffset::RdLen:
			pi.rd_len = data;
			InitDma<DmaType::RdramToCart>();
			break;

		case RegOffset::WrLen:
			pi.wr_len = data;
			InitDma<DmaType::CartToRdram>();
			break;

		case RegOffset::Status: {
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
		} break;

		case RegOffset::BsdDom1Lat: pi.bsd_dom1_lat = data; break;
		case RegOffset::BsdDom1Pwd: pi.bsd_dom1_pwd = data; break;
		case RegOffset::BsdDom1Pgs: pi.bsd_dom1_pgs = data; break;
		case RegOffset::BsdDom1Rls: pi.bsd_dom1_rls = data; break;
		case RegOffset::BsdDom2Lat: pi.bsd_dom2_lat = data; break;
		case RegOffset::BsdDom2Pwd: pi.bsd_dom2_pwd = data; break;
		case RegOffset::BsdDom2Pgs: pi.bsd_dom2_pgs = data; break;
		case RegOffset::BsdDom2Rls: pi.bsd_dom2_rls = data; break;

		default: std::unreachable();
		}
	}
}