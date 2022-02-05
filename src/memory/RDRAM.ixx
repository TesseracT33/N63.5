export module RDRAM;

import MemoryUtils;
import NumericalTypes;

import <algorithm>;
import <cassert>;
import <vector>;

namespace RDRAM
{
	constexpr size_t rdram_standard_size = 0x400000;
	constexpr size_t rdram_expanded_size = 0x800000;

	std::vector<u8> RDRAM(rdram_standard_size, 0);

	export
	{
		void AllocateExpansionPackRam()
		{
			RDRAM.resize(rdram_expanded_size);
			std::fill(RDRAM.begin() + rdram_standard_size, RDRAM.end(), 0);
		}

		/* $0000'0000 - $0x003F'FFFF */
		template<std::size_t number_of_bytes>
		auto ReadStandardRegion(const u32 address)
		{
			return MemoryUtils::GenericRead<number_of_bytes>(&RDRAM[address]);
		}

		/* $0040'0000 - $007F'FFFF */
		template<std::size_t number_of_bytes>
		auto ReadExpandedRegion(const u32 address)
		{
			if (RDRAM.size() != rdram_expanded_size)
				return MemoryUtils::ConstructUnsignedIntegral<number_of_bytes>(0);
			else
				return MemoryUtils::GenericRead<number_of_bytes>(&RDRAM[address]);
		}

		/* $03F0'0000 - $03FF'FFFF */
		template<std::size_t number_of_bytes>
		auto ReadRegisterRegion(const u32 address)
		{
			assert(false);
			return MemoryUtils::ConstructUnsignedIntegral<number_of_bytes>(0);
		}

		/* $0000'0000 - $0x003F'FFFF */
		template<std::size_t number_of_bytes>
		void WriteStandardRegion(const u32 address, const auto data)
		{
			MemoryUtils::GenericWrite<number_of_bytes>(&RDRAM[address], data);
		}

		/* $0040'0000 - $007F'FFFF */
		template<std::size_t number_of_bytes>
		void WriteExpandedRegion(const u32 address, const auto data)
		{
			if (RDRAM.size() == rdram_expanded_size)
				return;
			else
				MemoryUtils::GenericWrite<number_of_bytes>(&RDRAM[address], data);
		}

		/* $03F0'0000 - $03FF'FFFF */
		template<std::size_t number_of_bytes>
		void WriteRegisterRegion(const u32 address, const auto data)
		{
			assert(false);
		}
	}
}