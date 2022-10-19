export module RSP:Interface;

import Util;

import <algorithm>;
import <concepts>;
import <cstring>;
import <format>;
import <string>;
import <string_view>;

namespace RSP
{
	export
	{
		template<std::integral Int>
		Int CPUReadRegister(u32 addr);

		template<size_t number_of_bytes>
		void CPUWriteRegister(u32 addr, auto data);
	}

	enum class DmaType {
		RdToSp, SpToRd
	};

	enum RegOffset {
		DmaSpaddr, DmaRamaddr, DmaRdlen, DmaWrlen, Status, DmaFull, DmaBusy, Semaphore
	};

	template<DmaType dma_type> void InitDMA();
	void OnDmaFinish();

	struct
	{
		s32 dma_spaddr, dma_ramaddr, dma_rdlen, dma_wrlen,
			status, dma_full, dma_busy, semaphore;
	} regs;

	constexpr s32 sp_pc_addr = 0x0408'0000;

	bool dma_in_progress;
	bool dma_is_pending;

	/* What was written last to either SP_DMA_RDLEN/ SP_DMA_WRLEN during an ongoing DMA */
	s32 buffered_dma_rdlen;
	s32 buffered_dma_wrlen;

	DmaType in_progress_dma_type;

	void(*init_pending_dma_fun_ptr)();
}