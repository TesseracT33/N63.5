export module PI; /* Peripheral Interface */

import NumericalTypes;

import <algorithm>;
import <concepts>;
import <cstring>;
import <utility>;

namespace PI
{
	struct
	{
		s32 dram_addr, cart_addr, rd_len, wr_len, status;
		s32 bsd_dom1_lat, bsd_dom1_pwd, bsd_dom1_pgs, bsd_dom1_rls;
		s32 bsd_dom2_lat, bsd_dom2_pwd, bsd_dom2_pgs, bsd_dom2_rls;
		s32 dummy_reg_at_0x34, dummy_reg_at_0x38, dummy_reg_at_0x3C;
	} pi{};

	export
	{
		enum class StatusFlag : s32
		{
			DMA_BUSY      = 1 << 0,
			IO_BUSY       = 1 << 1,
			DMA_ERROR     = 1 << 2,
			DMA_COMPLETED = 1 << 3
		};

		void Initialize();

		template<StatusFlag status_flag>
		void SetStatusFlag();

		template<StatusFlag status_flag>
		void ClearStatusFlag();

		template<std::integral Int>
		Int Read(u32 addr);

		template<std::size_t number_of_bytes>
		void Write(u32 addr, auto data);
	}
}