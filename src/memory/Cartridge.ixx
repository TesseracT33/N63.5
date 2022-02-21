export module Cartridge;

import NumericalTypes;

import <cassert>;
import <concepts>;
import <fstream>;
import <string>;
import <vector>;

namespace Cartridge
{
	constexpr std::size_t rom_region_size = 0x0FC0'0000;
	constexpr std::size_t sram_region_size = 0x0800'0000;

	std::vector<u8> rom{};
	std::vector<u8> sram{};

	export
	{
		bool LoadROM(const std::string& rom_path);

		u8* GetPointerToROM(const u32 addr);

		std::size_t GetNumberOfBytesUntilRegionEnd(const u32 addr);

		template<std::integral Int>
		Int ReadROM(const u32 addr);

		template<std::integral Int>
		Int ReadSRAM(const u32 addr);

		template<std::size_t number_of_bytes>
		void WriteROM(const u32 addr, const auto data);

		template<std::size_t number_of_bytes>
		void WriteSRAM(const u32 addr, const auto data);
	}
}