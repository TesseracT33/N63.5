export module DMA;

import NumericalTypes;

import <algorithm>;
import <cassert>;
import <format>;
import <string>;
import <string_view>;

namespace DMA
{
	export
	{
		enum class Type
		{
			PI, SI
		};

		enum class Location
		{
			Cartridge, PIF, RDRAM
		};

		template<Type type, Location source, Location dest>
		void Init(std::size_t length, s32 source_start_addr, s32 dest_start_addr);
	}

	template<Location location>
	u8* GetPointerFromAddress(u32 addr);

	template<Location location>
	std::size_t GetNumberOfBytesUntilRegionEnd(u32 addr);

	constexpr std::string_view LocationToString(Location loc);
}