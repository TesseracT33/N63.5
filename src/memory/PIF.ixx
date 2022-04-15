export module PIF;

import NumericalTypes;

import <algorithm>;
import <array>;
import <concepts>;
import <format>;
import <fstream>;
import <optional>;
import <string>;

namespace PIF
{
	constexpr std::size_t ram_size = 0x40;
	constexpr std::size_t rom_size = 0x7C0;

	std::array<u8, ram_size> ram{};
	std::array<u8, rom_size> rom{};

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