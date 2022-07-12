export module RDRAM;

import NumericalTypes;

import <algorithm>;
import <bit>;
import <cassert>;
import <concepts>;
import <cstring>;
import <vector>;

namespace RDRAM
{
	export
	{
		void AllocateExpansionPackRam();
		void DeallocateExpansionPackRam();
		size_t GetNumberOfBytesUntilMemoryEnd(u32 start_addr);
		u8* GetPointerToMemory(u32 addr);

		/* $0000'0000 - $0x003F'FFFF */
		template<std::integral Int>
		Int ReadStandardRegion(u32 addr);

		/* $0040'0000 - $007F'FFFF */
		template<std::integral Int>
		Int ReadExpandedRegion(u32 addr);

		/* $03F0'0000 - $03FF'FFFF */
		template<std::integral Int>
		Int ReadRegisterRegion(u32 addr);

		/* $0000'0000 - $0x003F'FFFF */
		template<size_t number_of_bytes>
		void WriteStandardRegion(u32 addr, auto data);

		/* $0040'0000 - $007F'FFFF */
		template<size_t number_of_bytes>
		void WriteExpandedRegion(u32 addr, auto data);

		/* $03F0'0000 - $03FF'FFFF */
		template<size_t number_of_bytes>
		void WriteRegisterRegion(u32 addr, auto data);
	}

	constexpr size_t rdram_standard_size = 0x40'0000;
	constexpr size_t rdram_expanded_size = 0x80'0000;

	std::vector<u8> rdram(rdram_standard_size, 0);
}