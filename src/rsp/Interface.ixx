export module RSP:Interface;

import Util;

import <algorithm>;
import <bit>;
import <cstring>;
import <format>;
import <string>;
import <string_view>;
import <utility>;

namespace RSP
{
	export
	{
		s32 ReadReg(u32 addr);
		void WriteReg(u32 addr, s32 data);
	}

	enum class DmaType {
		RdToSp, SpToRd
	};

	enum Register {
		DmaSpaddr, DmaRamaddr, DmaRdlen, DmaWrlen, Status, DmaFull, DmaBusy, Semaphore
	};

	template<DmaType dma_type> void InitDMA();
	void OnDmaFinish();

	constexpr std::string_view RegOffsetToStr(u32 reg_offset);

	struct Sp {
		u32 dma_spaddr, dma_ramaddr, dma_rdlen, dma_wrlen;
		struct {
			u32 halted : 1;
			u32 broke : 1;
			u32 dma_busy : 1;
			u32 dma_full : 1;
			u32 io_busy : 1;
			u32 sstep : 1;
			u32 intbreak : 1;
			u32 sig : 8;
			u32 : 17;
		} status;
		u32 dma_full, dma_busy, semaphore;
	} sp;

	constexpr s32 sp_pc_addr = 0x0408'0000;

	bool dma_in_progress;
	bool dma_is_pending;

	/* What was written last to either SP_DMA_RDLEN/ SP_DMA_WRLEN during an ongoing DMA */
	s32 buffered_dma_rdlen;
	s32 buffered_dma_wrlen;
	s32 dma_spaddr_last_addr;
	s32 dma_ramaddr_last_addr;

	DmaType in_progress_dma_type;

	void(*init_pending_dma_fun_ptr)();
}