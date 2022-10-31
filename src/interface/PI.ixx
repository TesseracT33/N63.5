export module PI; /* Peripheral Interface */

import Util;

import <algorithm>;
import <cstring>;
import <format>;
import <utility>;

namespace PI
{
	export
	{
		enum class StatusFlag : s32 {
			DmaBusy      = 1 << 0,
			IoBusy       = 1 << 1,
			DmaError     = 1 << 2,
			DmaCompleted = 1 << 3
		};

		void ClearStatusFlag(StatusFlag);
		void Initialize();
		s32 ReadReg(u32 addr);
		void SetStatusFlag(StatusFlag);
		void WriteReg(u32 addr, s32 data);
	}

	enum class DmaType {
		CartToRdram, RdramToCart
	};

	template<DmaType> void InitDma(DmaType type);
	void OnDmaFinish();

	struct
	{
		s32 dram_addr, cart_addr, rd_len, wr_len, status;
		s32 bsd_dom1_lat, bsd_dom1_pwd, bsd_dom1_pgs, bsd_dom1_rls;
		s32 bsd_dom2_lat, bsd_dom2_pwd, bsd_dom2_pgs, bsd_dom2_rls;
		s32 dummy0, dummy1, dummy2;
	} pi;

	size_t dma_len;
}