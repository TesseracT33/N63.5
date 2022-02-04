export module RDRAM;

import MemoryUtils;
import NumericalTypes;

import <algorithm>;
import <cassert>;
import <concepts>;
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
		template<std::integral T>
		T ReadStandardRegion(const std::size_t number_of_bytes, const u32 address)
		{
			return MemoryUtils::GenericRead<T>(number_of_bytes, &RDRAM[address]);
		}

		/* $0040'0000 - $007F'FFFF */
		template<std::integral T>
		T ReadExpandedRegion(const std::size_t number_of_bytes, const u32 address)
		{
			if (RDRAM.size() != rdram_expanded_size)
				return 0;
			else
				return MemoryUtils::GenericRead<T>(number_of_bytes, &RDRAM[address]);
		}

		/* $03F0'0000 - $03FF'FFFF */
		template<std::integral T>
		T ReadRegisterRegion(const std::size_t number_of_bytes, const u32 address)
		{
			assert(false);
			return 0;
		}

		/* $0000'0000 - $0x003F'FFFF */
		template<std::integral T>
		void WriteStandardRegion(const std::size_t number_of_bytes, const u32 address, const T data)
		{
			MemoryUtils::GenericWrite<T>(number_of_bytes, &RDRAM[address], data);
		}

		/* $0040'0000 - $007F'FFFF */
		template<std::integral T>
		void WriteExpandedRegion(const std::size_t number_of_bytes, const u32 address, const T data)
		{
			if (RDRAM.size() == rdram_expanded_size)
				return;
			else
				MemoryUtils::GenericWrite<T>(number_of_bytes, &RDRAM[address], data);
		}

		/* $03F0'0000 - $03FF'FFFF */
		template<std::integral T>
		void WriteRegisterRegion(const std::size_t number_of_bytes, const u32 address, const T data)
		{
			assert(false);
		}
	}
}