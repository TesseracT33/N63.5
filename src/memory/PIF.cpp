module PIF;

import FileUtils;
import Memory;
import UserMessage;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace PIF
{
	bool LoadIPL12(const std::string& path)
	{
		const std::optional<std::array<u8, rom_size>> optional_rom = FileUtils::LoadBinaryFileArray<rom_size>(path);
		if (!optional_rom.has_value())
		{
			UserMessage::Show("Failed to open boot rom (IPL) file.", UserMessage::Type::Warning);
			return false;
		}
		rom = optional_rom.value();
		return true;
	}


	u8* GetPointerToRAM(const u32 offset)
	{
		return ram.data() + (offset & (ram.size() - 1)); /* Size is 0x40 bytes */
	}


	std::size_t GetNumberOfBytesUntilRAMEnd(const u32 offset)
	{
		return ram.size() - (offset & (ram.size() - 1)); /* Size is 0x40 bytes */
	}


	template<std::integral Int>
	Int ReadROM(const u32 offset)
	{
		return Memory::GenericRead<Int>(&rom[offset]);
	}


	template<std::integral Int>
	Int ReadRAM(const u32 offset)
	{
		return Memory::GenericRead<Int>(&ram[offset]);
	}


	template<std::size_t number_of_bytes>
	void WriteRAM(const u32 offset, const auto data)
	{
		Memory::GenericWrite<number_of_bytes>(&ram[offset], data);
	}



	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadROM, const u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadRAM, const u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteRAM, const u32);
}