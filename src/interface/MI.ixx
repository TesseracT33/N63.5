export module MI; /* MIPS Interface */

import NumericalTypes;


import <bit>;
import <concepts>;
import <cstring>;
import <utility>;

namespace MI
{
	export enum class InterruptType : s32
	{
		SP = 1 << 0, /* Set by the RSP when requested by a write to the SP status register, and optionally when the RSP halts */
		SI = 1 << 1, /* Set when a SI DMA to/from PIF RAM finishes */
		AI = 1 << 2, /* Set when no more samples remain in an audio stream */
		VI = 1 << 3, /* Set when VI_V_CURRENT == VI_V_INTR */
		PI = 1 << 4, /* Set when a PI DMA finishes */
		DP = 1 << 5  /* Set when a full sync completes */
	};

	struct
	{
		s32 mode, version, interrupt, mask;
	} mi{};

	template<InterruptType interrupt_type>
	void SetInterruptMask();

	template<InterruptType interrupt_type>
	void ClearInterruptMask();

	export
	{
		void Initialize();

		template<InterruptType interrupt_type>
		void SetInterruptFlag();

		template<InterruptType interrupt_type>
		void ClearInterruptFlag();

		template<std::integral Int>
		Int Read(u32 addr);

		template<std::size_t number_of_bytes>
		void Write(u32 addr, auto data);
	}
}