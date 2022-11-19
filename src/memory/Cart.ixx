export module Cart;

import Util;

import <algorithm>;
import <bit>;
import <cassert>;
import <concepts>;
import <cstring>;
import <format>;
import <optional>;
import <string>;
import <vector>;

namespace Cart
{
	export
	{
		size_t GetNumberOfBytesUntilRomEnd(u32 addr);
		u8* GetPointerToRom(u32 addr);
		u8* GetPointerToSram(u32 addr);
		bool LoadRom(const std::string& rom_path);
		bool LoadSram(const std::string& sram_path);

		template<std::signed_integral Int>
		Int ReadRom(u32 addr);

		template<std::signed_integral Int>
		Int ReadSram(u32 addr);

		template<size_t access_size>
		void WriteSram(u32 addr, s64 data);

		template<size_t access_size>
		void WriteRom(u32 addr, s64 data);
	}

	void AllocateSram();
	void ResizeRomToPowerOfTwo();

	constexpr size_t rom_region_size = 0x0FC0'0000;
	constexpr size_t sram_size = 0x10000; 

	u32 original_rom_size;
	u32 rom_access_mask;

	std::vector<u8> sram;
	std::vector<u8> rom;
}