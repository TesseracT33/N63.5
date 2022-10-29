export module PI; /* Peripheral Interface */

import Util;

import <algorithm>;
import <cstring>;
import <utility>;

namespace PI
{
	export
	{
		enum class StatusFlag : s32
		{
			DmaBusy      = 1 << 0,
			IoBusy       = 1 << 1,
			DmaError     = 1 << 2,
			DmaCompleted = 1 << 3
		};

		void ClearStatusFlag(StatusFlag);
		void Initialize();
		s32 ReadWord(u32 addr);
		void SetStatusFlag(StatusFlag);
		void WriteWord(u32 addr, s32 data);
	}

	struct
	{
		s32 dram_addr, cart_addr, rd_len, wr_len, status;
		s32 bsd_dom1_lat, bsd_dom1_pwd, bsd_dom1_pgs, bsd_dom1_rls;
		s32 bsd_dom2_lat, bsd_dom2_pwd, bsd_dom2_pgs, bsd_dom2_rls;
		s32 dummy_reg_at_0x34, dummy_reg_at_0x38, dummy_reg_at_0x3C;
	} pi{};
}