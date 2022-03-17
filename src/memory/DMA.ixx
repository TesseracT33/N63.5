export module DMA;

import NumericalTypes;

import <algorithm>;
import <cassert>;
import <format>;
import <string>;

namespace DMA
{
	export
	{
		enum class Location
		{
			RDRAM, Cartridge
		};

		template<Location source, Location dest>
		void Init(size_t length, u32 source_start_addr, u32 dest_start_addr);
	}

	template<Location location>
	u8* GetPointerFromAddress(u32 addr);

	template<Location location>
	size_t GetNumberOfBytesUntilRegionEnd(u32 addr);

	constexpr std::string_view LocationToString(Location loc);
}