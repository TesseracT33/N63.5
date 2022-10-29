export module DMA;

import Util;

import <algorithm>;
import <array>;
import <cassert>;
import <format>;
import <string>;
import <string_view>;
import <utility>;

namespace DMA
{
	export
	{
		enum class Location {
			Cartridge, PIF, RDRAM
		};

		enum class Type {
			PI, SI
		};

		template<Type type, Location src, Location dst>
		void Init(size_t length, s32 src_start_addr, s32 dst_start_addr);
	}

	template<Location>
	size_t GetNumberOfBytesUntilMemoryEnd(u32 addr);

	template<Location>
	u8* GetPointerFromAddress(u32 addr);

	constexpr std::array location_to_string_table = {
		"CART", "PIF", "RDRAM"
	};
}