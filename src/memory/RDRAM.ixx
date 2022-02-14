export module RDRAM;

import NumericalTypes;

import <algorithm>;
import <cassert>;
import <concepts>;
import <vector>;

namespace RDRAM
{
	constexpr std::size_t rdram_standard_size = 0x400000;
	constexpr std::size_t rdram_expanded_size = 0x800000;

	u32 rdram_read_mask = rdram_standard_size - 1;

	std::vector<u8> RDRAM(rdram_standard_size, 0);

	export
	{
		void AllocateExpansionPackRam();
		void DeallocateExpansionPackRam();

		/* $0000'0000 - $0x003F'FFFF */
		template<std::integral Int>
		Int ReadStandardRegion(const u32 address);

		/* $0040'0000 - $007F'FFFF */
		template<std::integral Int>
		Int ReadExpandedRegion(const u32 address);

		/* $03F0'0000 - $03FF'FFFF */
		template<std::integral Int>
		Int ReadRegisterRegion(const u32 address);

		/* $0000'0000 - $0x003F'FFFF */
		template<std::size_t number_of_bytes>
		void WriteStandardRegion(const u32 address, const auto data);

		/* $0040'0000 - $007F'FFFF */
		template<std::size_t number_of_bytes>
		void WriteExpandedRegion(const u32 address, const auto data);

		/* $03F0'0000 - $03FF'FFFF */
		template<std::size_t number_of_bytes>
		void WriteRegisterRegion(const u32 address, const auto data);

		u8* GetPointer(const u32 addr);

		std::size_t GetNumberOfBytesUntilRegionEnd(const u32 start_addr);
	}
}