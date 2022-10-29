export module Cartridge;

import Util;

import <bit>;
import <cassert>;
import <concepts>;
import <cstring>;
import <optional>;
import <string>;
import <vector>;

namespace Cartridge
{
	export
	{
		size_t GetNumberOfBytesUntilROMEnd(u32 addr);
		u8* GetPointerToROM(u32 addr);
		u8* GetPointerToSRAM(u32 addr);
		bool LoadROM(const std::string& rom_path);
		bool LoadSRAM(const std::string& ram_path);

		template<std::signed_integral Int>
		Int ReadROM(u32 addr);

		template<std::signed_integral Int>
		Int ReadSRAM(u32 addr);

		template<size_t num_bytes>
		void WriteSRAM(u32 addr, std::signed_integral auto data);
	}

	constexpr size_t rom_region_size = 0x0FC0'0000;
	constexpr size_t sram_region_size = 0x0800'0000;

	std::vector<u8> rom{};
	std::vector<u8> sram{};
}