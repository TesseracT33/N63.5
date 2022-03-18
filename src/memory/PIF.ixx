export module PIF;

import NumericalTypes;

import <algorithm>;
import <array>;
import <concepts>;
import <format>;
import <fstream>;
import <string>;

namespace PIF
{
	std::array<u8, 0x7C0> pif_rom{};
	std::array<u8, 0x40> pif_ram{};

	export
	{
		bool LoadIPL12(const std::string& path);

		template<std::integral Int>
		Int ReadROM(const u32 offset);

		template<std::integral Int>
		Int ReadRAM(const u32 offset);

		template<std::size_t number_of_bytes>
		void WriteRAM(const u32 offset, const auto data);
	}
}