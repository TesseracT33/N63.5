module PI;

import Cart;
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

		u8* rdram_ptr = RDRAM::GetPointerToMemory(pi.dram_addr);
		u8* cart_ptr = Cart::GetPointerToRom(pi.cart_addr);
		size_t bytes_until_rdram_end = RDRAM::GetNumberOfBytesUntilMemoryEnd(pi.dram_addr);
		size_t bytes_until_cart_end = Cart::GetNumberOfBytesUntilRomEnd(pi.cart_addr);
		dma_len = std::min(bytes_until_rdram_end, bytes_until_cart_end);
		if constexpr (type == DmaType::CartToRdram) {
			dma_len = std::min(dma_len, size_t(pi.wr_len + 1));
			/* See https://n64brew.dev/wiki/Peripheral_Interface#Unaligned_DMA_transfer for behavior when addr is unaligned */
			static constexpr size_t block_size = 128;
			size_t num_bytes_first_block = block_size - (pi.dram_addr & (block_size - 1));
			if (num_bytes_first_block > (pi.dram_addr & 7)) {
				std::memcpy(rdram_ptr, cart_ptr, std::min(dma_len, num_bytes_first_block - (pi.dram_addr & 7)));
			}
			if (dma_len > num_bytes_first_block) {
				std::memcpy(rdram_ptr + num_bytes_first_block, cart_ptr + num_bytes_first_block, dma_len - num_bytes_first_block);
			}
			if constexpr (log_dma) {
				LogDma(std::format("From cart ROM ${:X} to RDRAM ${:X}: ${:X} bytes",
					pi.cart_addr, pi.dram_addr, dma_len));
			}
		}
		else { /* RDRAM to cart */
			/* TODO: when I wrote this code, I forgot we can't write to ROM. But it seems we can write to SRAM/FLASH.
				I do not yet know the behavior */
			//dma_len = std::min(dma_len, size_t(pi.rd_len + 1));
			//std::memcpy(cart_ptr, rdram_ptr, dma_len);
			//if constexpr (log_dma) {
			//	LogDMA(std::format("From RDRAM ${:X} to cart ROM ${:X}: ${:X} bytes",
			//		pi.dram_addr, pi.cart_addr, dma_len));
			//}
			Log("Attempted DMA from RDRAM to Cart, but this is unimplemented.");
			OnDmaFinish();
			return;
		}

		static constexpr auto cycles_per_byte_dma = 18;
		auto cycles_until_finish = dma_len * cycles_per_byte_dma;
		Scheduler::AddEvent(Scheduler::EventType::PiDmaFinish, cycles_until_finish, OnDmaFinish);
	}


	void Initialize()
	{
		std::memset(&pi, 0, sizeof(pi));
	}


	void OnDmaFinish()
	{
		SetStatusFlag(StatusFlag::DmaCompleted);
		ClearStatusFlag(StatusFlag::DmaBusy);
		MI::SetInterruptFlag(MI::InterruptType::PI);
		pi.dram_addr = (pi.dram_addr + dma_len) & 0xFF'FFFF;
		pi.cart_addr = (pi.cart_addr + dma_len) & 0xFF'FFFF;
	}


	s32 ReadReg(u32 addr)
	{
		static_assert(sizeof(pi) >> 2 == 0x10);
		u32 offset = addr >> 2 & 0xF;
		s32 ret;
		std::memcpy(&ret, (s32*)(&pi) + offset, 4);
		if constexpr (log_io_ai) {
			LogIoRead("PI", RegOffsetToStr(offset), ret);
		}
		return ret;
	}


	constexpr std::string_view RegOffsetToStr(u32 reg_offset)
	{
		switch (reg_offset) {
		case DramAddr: return "PI_DRAM_ADDR";
		case CartAddr: return "PI_CART_ADDR";
		case RdLen: return "PI_RDLEN";
		case WrLen: return "PI_WRLEN";
		case Status: return "PI_STATUS";
		case BsdDom1Lat: return "PI_BSDDOM1LAT";
		case BsdDom1Pwd: return "PI_BSDDOM1PWD";
		case BsdDom1Pgs: return "PI_BSDDOM1PGS";
		case BsdDom1Rls: return "PI_BSDDOM1RLS";
		case BsdDom2Lat: return "PI_BSDDOM2LAT";
		case BsdDom2Pwd: return "PI_BSDDOM2PWD";
		case BsdDom2Pgs: return "PI_BSDDOM2PGS";
		case BsdDom2Rls: return "PI_BSDDOM2RLS";
		default: std::unreachable();
		}
	}


	void SetStatusFlag(StatusFlag status_flag)
	{
		pi.status |= std::to_underlying(status_flag);
	}


	void WriteReg(u32 addr, s32 data)
	{
		static_assert(sizeof(pi) >> 2 == 0x10);
		u32 offset = addr >> 2 & 0xF;
		if constexpr (log_io_ai) {
			LogIoWrite("PI", RegOffsetToStr(offset), data);
		}

		switch (offset) {
		case Register::DramAddr:
			pi.dram_addr = data & 0xFF'FFFF;
			break;

		case Register::CartAddr:
			pi.cart_addr = data & 0xFF'FFFF;
			break;

		case Register::RdLen:
			pi.rd_len = data;
			InitDma<DmaType::RdramToCart>();
			break;

		case Register::WrLen:
			pi.wr_len = data;
			InitDma<DmaType::CartToRdram>();
			break;

		case Register::Status: {
			constexpr static s32 reset_dma_mask = 0x01;
			constexpr static s32 clear_interrupt_mask = 0x02;
			if (data & reset_dma_mask) {
				/* Reset the DMA controller and stop any transfer being done */
				pi.status = 0;
				MI::ClearInterruptFlag(MI::InterruptType::PI); /* TODO: correct? */
				Scheduler::RemoveEvent(Scheduler::EventType::PiDmaFinish);
			}
			if (data & clear_interrupt_mask) {
				/* Clear Interrupt */
				ClearStatusFlag(StatusFlag::DmaCompleted);
				MI::ClearInterruptFlag(MI::InterruptType::PI);
			}
		} break;

		case Register::BsdDom1Lat: pi.bsd_dom1_lat = data; break;
		case Register::BsdDom1Pwd: pi.bsd_dom1_pwd = data; break;
		case Register::BsdDom1Pgs: pi.bsd_dom1_pgs = data; break;
		case Register::BsdDom1Rls: pi.bsd_dom1_rls = data; break;
		case Register::BsdDom2Lat: pi.bsd_dom2_lat = data; break;
		case Register::BsdDom2Pwd: pi.bsd_dom2_pwd = data; break;
		case Register::BsdDom2Pgs: pi.bsd_dom2_pgs = data; break;
		case Register::BsdDom2Rls: pi.bsd_dom2_rls = data; break;

		default:
			Log(std::format("Unexpected write made to PI register at address ${:08X}", addr));
		}
	}
}