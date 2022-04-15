module Cartridge;

import FileUtils;
import Memory;
import UserMessage;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace Cartridge
{
	bool LoadROM(const std::string& rom_path)
	{
		const std::optional<std::vector<u8>> optional_rom = FileUtils::LoadBinaryFileVec(rom_path);
		if (!optional_rom.has_value())
		{
			UserMessage::Show("Failed to open rom file.", UserMessage::Type::Error);
			return false;
		}
		rom = optional_rom.value();
		return true;
	}


	u8* GetPointerToROM(const u32 addr)
	{
		const u32 rom_offset = (addr & 0x0FFF'FFFF) % rom.size();
		return rom.data() + rom_offset;
	}


	std::size_t GetNumberOfBytesUntilROMEnd(const u32 addr)
	{
		const u32 rom_offset = (addr & 0x0FFF'FFFF) % rom.size();
		return rom.size() - rom_offset;
	}


	template<std::integral Int>
	Int ReadROM(const u32 addr)
	{
		const u32 read_offset = (addr & 0x0FFF'FFFF) % rom.size();
		return Memory::GenericRead<Int>(rom.data() + read_offset);
	}


	template<std::integral Int>
	Int ReadSRAM(const u32 addr)
	{
		if (sram.size() == 0)
			return 0;
		const u32 read_offset = (addr & 0x0FFF'FFFF) % sram.size();
		return Memory::GenericRead<Int>(sram.data() + read_offset);
	}


	template<std::size_t number_of_bytes>
	void WriteROM(const u32 addr, const auto data)
	{
		assert(false);
	}


	template<std::size_t number_of_bytes>
	void WriteSRAM(const u32 addr, const auto data)
	{
		if (sram.size() == 0)
			return;
		const u32 write_offset = (addr & 0x0FFF'FFFF) % sram.size();
		Memory::GenericWrite<number_of_bytes>(sram.data() + write_offset, data);
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadROM, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadSRAM, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteROM, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteSRAM, const u32)
}