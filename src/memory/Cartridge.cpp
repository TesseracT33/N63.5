module Cartridge;

import UserMessage;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace Cartridge
{
	bool LoadROM(const std::string& rom_path)
	{
		/* Attempt to open the rom file */
		std::ifstream ifs{ rom_path, std::ifstream::in | std::ifstream::binary };
		if (!ifs)
		{
			UserMessage::Show("Failed to open rom file.", UserMessage::Type::Error);
			return false;
		}

		/* Compute the rom file size and resize the rom vector */
		ifs.seekg(0, ifs.end);
		const size_t rom_size = ifs.tellg();
		rom.resize(rom_size);

		/* Read the file */
		ifs.seekg(0, ifs.beg);
		ifs.read((char*)rom.data(), rom_size);

		return true;
	}


	u8* GetPointerToROM(const u32 addr)
	{
		return rom.data() + (addr % rom.size());
	}


	std::size_t GetNumberOfBytesUntilRegionEnd(const u32 start_addr)
	{
		return rom.size() - (start_addr % rom.size());
	}


	template<std::integral Int>
	Int ReadROM(const u32 addr)
	{
		return Int(0);
	}


	template<std::integral Int>
	Int ReadSRAM(const u32 addr)
	{
		return Int(0);
	}


	template<std::size_t number_of_bytes>
	void WriteROM(const u32 addr, const auto data)
	{
		assert(false);
	}


	template<std::size_t number_of_bytes>
	void WriteSRAM(const u32 addr, const auto data)
	{

	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadROM, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadSRAM, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteROM, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteSRAM, const u32)
}