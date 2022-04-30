export module Cartridge;

import NumericalTypes;

import <bit>;
import <cassert>;
import <concepts>;
import <cstring>;
import <optional>;
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
		bool LoadSRAM(const std::string& ram_path);

		u8* GetPointerToROM(u32 addr);
		u8* GetPointerToSRAM(u32 addr);

		std::size_t GetNumberOfBytesUntilROMEnd(u32 addr);

		template<std::integral Int>
		Int ReadROM(u32 addr);

		template<std::integral Int>
		Int ReadSRAM(u32 addr);

		template<std::size_t number_of_bytes>
		void WriteROM(u32 addr, auto data);

		template<std::size_t number_of_bytes>
		void WriteSRAM(u32 addr, auto data);
	}
}