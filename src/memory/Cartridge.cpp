module Cartridge;

import Memory;
import UserMessage;

import Util;

#include "../EnumerateTemplateSpecializations.h"

namespace Cartridge
{
	size_t GetNumberOfBytesUntilROMEnd(const u32 addr)
	{
		u32 offset = (addr & 0x0FFF'FFFF) % rom.size();
		return rom.size() - offset;
	}


	u8* GetPointerToROM(const u32 addr)
	{
		if (rom.size() == 0) {
			return nullptr;
		}
		u32 offset = (addr & 0x0FFF'FFFF) % rom.size();
		return rom.data() + offset;
	}


	u8* GetPointerToSRAM(const u32 addr)
	{
		if (sram.size() == 0) {
			return nullptr;
		}
		u32 offset = (addr & 0x0FFF'FFFF) % sram.size();
		return sram.data() + offset;
	}


	bool LoadROM(const std::string& rom_path)
	{
		std::optional<std::vector<u8>> optional_rom = ReadFileIntoVector(rom_path);
		if (!optional_rom.has_value()) {
			UserMessage::Show("Failed to open rom file.", UserMessage::Type::Error);
			return false;
		}
		rom = optional_rom.value();
		Memory::ReloadPageTables();
		return true;
	}


	bool LoadSRAM(const std::string& ram_path)
	{
		std::optional<std::vector<u8>> optional_sram = ReadFileIntoVector(ram_path);
		if (!optional_sram.has_value()) {
			return false;
		}
		sram = optional_sram.value();
		Memory::ReloadPageTables();
		return true;
	}


	template<std::integral Int>
	Int ReadROM(const u32 addr)
	{
		Int ret;
		std::memcpy(&ret, GetPointerToROM(addr), sizeof(Int));
		return std::byteswap(ret);
	}


	template<std::integral Int>
	Int ReadSRAM(const u32 addr)
	{ /* CPU precondition: addr is always aligned */
		if (sram.size() == 0) {
			return 0;
		}
		else {
			Int ret;
			std::memcpy(&ret, GetPointerToSRAM(addr), sizeof(Int));
			return std::byteswap(ret);
		}
	}


	template<size_t number_of_bytes>
	void WriteROM(const u32 addr, const auto data)
	{
		assert(false);
	}


	template<size_t number_of_bytes>
	void WriteSRAM(const u32 addr, auto data)
	{ /* CPU precondition: addr + number_of_bytes does not go beyond the next alignment boundary */
		if (sram.size() != 0) {
			data = std::byteswap(data);
			std::memcpy(GetPointerToSRAM(addr), &data, number_of_bytes);
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadROM, u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadSRAM, u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteROM, u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteSRAM, u32)
}