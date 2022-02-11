module Cartridge;

import Memory;
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
		const std::size_t rom_size = ifs.tellg();
		rom_size_mask = u32( [&] {
			if (rom_size <= rom_region_size) return rom_size - 1; /* TODO: can we assume that N64 rom sizes are always a power of 2? */
			else return rom_region_size;
		}());
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
		const u32 read_offset = addr & rom_size_mask;
		const Int ret = Memory::GenericRead<Int>(rom.data() + read_offset);
		return Memory::Byteswap(ret);
	}


	template<std::integral Int>
	Int ReadSRAM(const u32 addr)
	{
		if (sram.size() == 0)
			return 0;
		const u32 read_offset = addr & sram_size_mask;
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
		const u32 write_offset = addr & sram_size_mask;
		Memory::GenericWrite<number_of_bytes>(sram.data() + write_offset, data);
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadROM, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadSRAM, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteROM, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteSRAM, const u32)
}