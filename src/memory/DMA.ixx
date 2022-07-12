export module DMA;

import NumericalTypes;

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
			Cartridge, PIF, RDRAM, SPRAM
		};

		enum class Type {
			PI, SI, SP
		};

		template<Type type, Location source, Location dest>
		void Init(size_t length, s32 source_start_addr, s32 dest_start_addr);

		template<Type type, Location source, Location dest>
		void Init(size_t rows, size_t bytes_per_row, size_t skip, s32 source_start_addr, s32 dest_start_addr);
	}

	template<Location>
	size_t GetNumberOfBytesUntilMemoryEnd(u32 addr);

	template<Location>
	u8* GetPointerFromAddress(u32 addr);

	constexpr std::array location_to_string_table = {
		"CART", "PIF", "RDRAM", "DMEM/IMEM"
	};
}