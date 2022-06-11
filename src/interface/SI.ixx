export module SI; /* Serial Interface */

import NumericalTypes;

import <concepts>;
import <cstring>;
import <utility>;

namespace SI
{
	struct
	{
		s32 dram_addr, pif_addr_rd64b, pif_addr_wr4b, dummy_reg_at_0x0C, pif_addr_wr64b, pif_addr_rd4b, status, dummy_reg_at_0x1C;
	} si{};

	export
	{
		enum class StatusFlag : s32
		{
			DMA_BUSY = 1 << 0, /* Set when a read or write DMA, or an IO write, is in progress. */
			IO_BUSY = 1 << 1, /* Set when either an IO read or write is in progress. */
			READ_PENDING = 1 << 2, /* Set when an IO read occurs, while an IO or DMA write is in progress. */
			DMA_ERROR = 1 << 3, /* Set when overlapping DMA requests occur. Can only be cleared with a power reset. */
			INTERRUPT = 1 << 12 /* Copy of SI interrupt flag from MIPS Interface, which is also seen in the RCP Interrupt Cause register. Writing any value to STATUS clears this bit in all three locations. */
		};

		void Initialize();

		template<StatusFlag status_flag>
		void SetStatusFlag();

		template<StatusFlag status_flag>
		void ClearStatusFlag();

		template<std::integral Int>
		Int Read(const u32 addr);

		template<std::size_t number_of_bytes>
		void Write(const u32 addr, const auto data);
	}
}