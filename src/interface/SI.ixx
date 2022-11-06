export module SI; /* Serial Interface */

import Util;

import <cstring>;
import <format>;
import <string_view>;
import <utility>;

namespace SI
{
	export
	{
		enum class StatusFlag : s32 {
			DmaBusy = 1 << 0, /* Set when a read or write DMA, or an IO write, is in progress. */
			IoBusy = 1 << 1, /* Set when either an IO read or write is in progress. */
			ReadPending = 1 << 2, /* Set when an IO read occurs, while an IO or DMA write is in progress. */
			DmaError = 1 << 3, /* Set when overlapping DMA requests occur. Can only be cleared with a power reset. */
			Interrupt = 1 << 12 /* Copy of SI interrupt flag from MIPS Interface, which is also seen in the RCP Interrupt Cause register. Writing any value to STATUS clears this bit in all three locations. */
		};

		void ClearStatusFlag(StatusFlag);
		void Initialize();
		s32 ReadReg(u32 addr);
		void SetStatusFlag(StatusFlag);
		void WriteReg(u32 addr, s32 data);
	}

	enum class DmaType {
		PifToRdram, RdramToPif
	};

	enum Register {
		DramAddr = 0, AddrRd64B = 1, AddrWr4B = 2,
		AddrWr64B = 4, AddrRd4B = 5, Status = 6
	};

	template<DmaType> void InitDma();
	void OnDmaFinish();
	constexpr std::string_view RegOffsetToStr(u32 reg_offset);

	struct {
		s32 dram_addr, pif_addr_rd64b, pif_addr_wr4b, dummy0,
			pif_addr_wr64b, pif_addr_rd4b, status, dummy1;
	} si;

	size_t dma_len;
	s32* pif_addr_reg_last_dma;
}