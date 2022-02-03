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
		void allocate_expansion_pack_ram()
		{
			RDRAM.resize(rdram_expanded_size);
			std::fill(RDRAM.begin() + rdram_standard_size, RDRAM.end(), 0);
		}

		/* $0000'0000 - $0x003F'FFFF */
		template<std::integral T>
		T read_standard_region(const std::size_t number_of_bytes, const u32 address)
		{
			return MemoryUtils::generic_read<T>(number_of_bytes, &RDRAM[address]);
		}

		/* $0040'0000 - $007F'FFFF */
		template<std::integral T>
		T read_expanded_region(const std::size_t number_of_bytes, const u32 address)
		{
			if (RDRAM.size() != rdram_expanded_size)
				return 0;
			else
				return MemoryUtils::generic_read<T>(number_of_bytes, &RDRAM[address]);
		}

		/* $03F0'0000 - $03FF'FFFF */
		template<std::integral T>
		T read_register_region(const std::size_t number_of_bytes, const u32 address)
		{
			assert(false);
			return 0;
		}

		/* $0000'0000 - $0x003F'FFFF */
		template<std::integral T>
		void write_standard_region(const std::size_t number_of_bytes, const u32 address, const T data)
		{
			MemoryUtils::generic_write<T>(number_of_bytes, &RDRAM[address], data);
		}

		/* $0040'0000 - $007F'FFFF */
		template<std::integral T>
		void write_expanded_region(const std::size_t number_of_bytes, const u32 address, const T data)
		{
			if (RDRAM.size() == rdram_expanded_size)
				return;
			else
				MemoryUtils::generic_write<T>(number_of_bytes, &RDRAM[address], data);
		}

		/* $03F0'0000 - $03FF'FFFF */
		template<std::integral T>
		void write_register_region(const std::size_t number_of_bytes, const u32 address, const T data)
		{
			assert(false);
		}
	}
}