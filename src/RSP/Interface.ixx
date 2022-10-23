export module RSP:Interface;

import Util;

import <algorithm>;
import <bit>;
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

	struct StatusRegister
	{
		u32 halted : 1;
		u32 broke : 1;
		u32 dma_busy : 1;
		u32 dma_full : 1;
		u32 io_busy : 1;
		u32 sstep : 1;
		u32 intbreak : 1;
		u32 sig : 8;
		u32 : 17;
	};

	struct
	{
		u32 dma_spaddr, dma_ramaddr, dma_rdlen, dma_wrlen;
		StatusRegister status;
		u32 dma_full, dma_busy, semaphore;
	} sp;

	constexpr s32 sp_pc_addr = 0x0408'0000;

	bool dma_in_progress;
	bool dma_is_pending;

	/* What was written last to either SP_DMA_RDLEN/ SP_DMA_WRLEN during an ongoing DMA */
	s32 buffered_dma_rdlen;
	s32 buffered_dma_wrlen;

	DmaType in_progress_dma_type;

	void(*init_pending_dma_fun_ptr)();
}