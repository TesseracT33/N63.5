export module RDRAM;

import Util;

import <algorithm>;
import <array>;
import <bit>;
import <cassert>;
import <concepts>;
import <cstring>;

namespace RDRAM
{
	export
	{
		size_t GetNumberOfBytesUntilMemoryEnd(u32 start_addr);
		u8* GetPointerToMemory(u32 addr = 0);
		size_t GetSize();

		/* 0 - $7F'FFFF */
		template<std::signed_integral Int>
		Int Read(u32 addr);

		/* $03F0'0000 - $03FF'FFFF */
		template<std::signed_integral Int>
		Int ReadRegisterRegion(u32 addr);

		u64 RspReadCommandByteswapped(u32 addr);

		/* 0 - $7F'FFFF */
		template<size_t num_bytes>
		void Write(u32 addr, std::signed_integral auto data);

		/* $03F0'0000 - $03FF'FFFF */
		template<size_t num_bytes>
		void WriteRegisterRegion(u32 addr, std::signed_integral auto data);
	}

	constexpr size_t rdram_standard_size = 0x40'0000;
	constexpr size_t rdram_expanded_size = 0x80'0000;

	/* Note: could not use std::array here as .data() does not become properly aligned */
	/* TODO: parallel-rdp required 4096 on my system. Investigate further. */
	alignas(4096) u8 rdram[rdram_expanded_size]; /* TODO: make it dynamic? */
}