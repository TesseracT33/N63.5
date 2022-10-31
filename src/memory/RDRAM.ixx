export module RDRAM;

import Util;

import <bit>;
import <concepts>;
import <cstring>;

namespace RDRAM
{
	export
	{
		size_t GetNumberOfBytesUntilMemoryEnd(u32 addr);
		u8* GetPointerToMemory(u32 addr = 0);
		size_t GetSize();
		void Initialize();
		template<std::signed_integral Int> Int Read(u32 addr);
		s32 ReadReg(u32 addr);
		u64 RspReadCommandByteswapped(u32 addr);
		template<size_t num_bytes> void Write(u32 addr, std::signed_integral auto data);
		void WriteReg(u32 addr, s32 data);
	}

	struct {
		u32 device_type, device_id, delay, mode, ref_interval, ref_row,
			ras_interval, min_interval, addr_select, device_manuf;
	} reg;

	constexpr size_t rdram_standard_size = 0x40'0000;
	constexpr size_t rdram_expanded_size = 0x80'0000;

	/* Note: could not use std::array here as .data() does not become properly aligned */
	/* TODO: parallel-rdp required 4096 on my system. Investigate further. */
	alignas(4096) u8 rdram[rdram_expanded_size]; /* TODO: make it dynamic? */
}