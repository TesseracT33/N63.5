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

		u8* GetPointerToRAM(u32 offset);

		std::size_t GetNumberOfBytesUntilRAMEnd(u32 offset);

		template<std::integral Int>
		Int ReadROM(u32 offset);

		template<std::integral Int>
		Int ReadRAM(u32 offset);

		template<std::size_t number_of_bytes>
		void WriteRAM(u32 offset, auto data);
	}
}