export module PIF;

import NumericalTypes;

import <algorithm>;
import <array>;
import <bit>;
import <concepts>;
import <cstring>;
import <optional>;
import <string>;

namespace PIF
{
	constexpr std::size_t ram_size = 0x40;
	constexpr std::size_t rom_size = 0x7C0;

	std::array<u8, rom_size + ram_size> memory{}; /* $0-$7BF: rom; $7C0-$7FF: ram $*/

	export
	{
		bool LoadIPL12(const std::string& path);

		u8* GetPointerToRAM(u32 address);
		u8* GetPointerToMemory(u32 address);

		std::size_t GetNumberOfBytesUntilRAMEnd(u32 offset);

		template<std::integral Int>
		Int ReadMemory(u32 addr);

		template<std::size_t number_of_bytes>
		void WriteMemory(u32 addr, auto data);
	}
}