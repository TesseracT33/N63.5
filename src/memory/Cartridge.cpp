module Cartridge;

import Memory;
import UserMessage;

import Util;

namespace Cartridge
{
	void AllocateSram()
	{
		sram.resize(sram_size);
		std::ranges::fill(sram, 0xFF);
		Memory::ReloadPageTables();
	}


	size_t GetNumberOfBytesUntilRomEnd(u32 addr)
	{
		static constexpr u32 addr_rom_start = 0x1000'0000;
		return original_rom_size - (addr - addr_rom_start) % original_rom_size;
	}


	u8* GetPointerToRom(u32 addr)
	{
		return rom.empty() ? nullptr : rom.data() + (addr & rom_access_mask);
	}


	u8* GetPointerToSram(u32 addr)
	{
		return sram.empty() ? nullptr : sram.data() + (addr & (sram_size - 1));
	}


	bool LoadRom(const std::string& rom_path)
	{
		std::optional<std::vector<u8>> optional_rom = ReadFileIntoVector(rom_path);
		if (!optional_rom.has_value()) {
			UserMessage::Show("Failed to open rom file.", UserMessage::Type::Error);
			return false;
		}
		rom = optional_rom.value();
		if (rom.empty()) {
			UserMessage::Show("Rom file has size 0.", UserMessage::Type::Error);
			return false;
		}
		if (rom.size() > rom_region_size) {
			UserMessage::Show(std::format("Rom file has size larger than the maximum allowed ({} bytes). "
				"Truncating to the maximum allowed.", rom_region_size), UserMessage::Type::Warning);
			rom.resize(rom_region_size);
		}
		original_rom_size = rom.size();
		ResizeRomToPowerOfTwo();
		rom_access_mask = u32(rom.size() - 1);
		Memory::ReloadPageTables();
		AllocateSram();
		return true;
	}


	bool LoadSram(const std::string& sram_path)
	{
		std::optional<std::vector<u8>> optional_sram = ReadFileIntoVector(sram_path);
		if (!optional_sram.has_value()) {
			return false;
		}
		sram = optional_sram.value();
		if (sram.empty()) {
			UserMessage::Show("Error: sram file has size 0.", UserMessage::Type::Error);
			return false;
		}
		if (sram.size() != sram_size) {
			UserMessage::Show(std::format("Sram file has size different than the allowed ({} bytes). ", sram_size), UserMessage::Type::Warning);
			sram.resize(sram_size);
		}
		Memory::ReloadPageTables();
		return true;
	}


	template<std::signed_integral Int>
	Int ReadRom(u32 addr)
	{
		Int ret;
		std::memcpy(&ret, GetPointerToRom(addr), sizeof(Int));
		return std::byteswap(ret);
	}


	template<std::signed_integral Int>
	Int ReadSram(u32 addr)
	{ /* CPU precondition: addr is always aligned */
		Int ret;
		std::memcpy(&ret, GetPointerToSram(addr), sizeof(Int));
		return std::byteswap(ret);
	}


	void ResizeRomToPowerOfTwo()
	{
		size_t actual_size = rom.size();
		size_t pow_two_size = std::bit_ceil(actual_size);
		size_t diff = pow_two_size - actual_size;
		if (diff > 0) {
			rom.resize(pow_two_size);
			std::copy(rom.begin(), rom.begin() + diff, rom.begin() + actual_size);
		}
	}


	template<size_t num_bytes>
	void WriteSram(u32 addr, std::signed_integral auto data)
	{ /* CPU precondition: addr + number_of_bytes does not go beyond the next alignment boundary */
		data = std::byteswap(data);
		std::memcpy(GetPointerToSram(addr), &data, num_bytes);
	}


	template s8 ReadRom<s8>(u32);
	template s16 ReadRom<s16>(u32);
	template s32 ReadRom<s32>(u32);
	template s64 ReadRom<s64>(u32);
	template s8 ReadSram<s8>(u32);
	template s16 ReadSram<s16>(u32);
	template s32 ReadSram<s32>(u32);
	template s64 ReadSram<s64>(u32);
	template void WriteSram<1>(u32, s8);
	template void WriteSram<1>(u32, s16);
	template void WriteSram<1>(u32, s32);
	template void WriteSram<1>(u32, s64);
	template void WriteSram<2>(u32, s16);
	template void WriteSram<2>(u32, s32);
	template void WriteSram<2>(u32, s64);
	template void WriteSram<3>(u32, s32);
	template void WriteSram<3>(u32, s64);
	template void WriteSram<4>(u32, s32);
	template void WriteSram<4>(u32, s64);
	template void WriteSram<5>(u32, s64);
	template void WriteSram<6>(u32, s64);
	template void WriteSram<7>(u32, s64);
	template void WriteSram<8>(u32, s64);
}