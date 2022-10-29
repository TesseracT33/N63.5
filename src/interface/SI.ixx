export module SI; /* Serial Interface */

import Util;

import <cstring>;
import <utility>;

namespace SI
{
	export
	{
		enum class StatusFlag : s32
		{
			DmaBusy = 1 << 0, /* Set when a read or write DMA, or an IO write, is in progress. */
			IoBusy = 1 << 1, /* Set when either an IO read or write is in progress. */
			ReadPending = 1 << 2, /* Set when an IO read occurs, while an IO or DMA write is in progress. */
			DmaError = 1 << 3, /* Set when overlapping DMA requests occur. Can only be cleared with a power reset. */
			Interrupt = 1 << 12 /* Copy of SI interrupt flag from MIPS Interface, which is also seen in the RCP Interrupt Cause register. Writing any value to STATUS clears this bit in all three locations. */
		};

		void ClearStatusFlag(StatusFlag);
		void Initialize();
		s32 ReadWord(u32 addr);
		void SetStatusFlag(StatusFlag);
		void WriteWord(u32 addr, s32 data);
	}

	struct
	{
		s32 dram_addr, pif_addr_rd64b, pif_addr_wr4b, dummy_reg_at_0x0C,
			pif_addr_wr64b, pif_addr_rd4b, status, dummy_reg_at_0x1C;
	} si{};
}