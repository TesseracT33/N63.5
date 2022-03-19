module PIF;

import Memory;
import UserMessage;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace PIF
{
	bool LoadIPL12(const std::string& path)
	{
		/* Attempt to open the file */
		std::ifstream ifs{ path, std::ifstream::in | std::ifstream::binary };
		if (!ifs)
		{
			UserMessage::Show("Failed to open boot rom (IPL) file.", UserMessage::Type::Warning);
			return false;
		}

		/* Compute the rom file size */
		ifs.seekg(0, ifs.end);
		const std::size_t size = ifs.tellg();
		/* TODO: NTSC boot rom is 0x7C0 bytes, while PAL is 0x7FF? */
		if (size != pif_rom.size())
		{
			UserMessage::Show(
				std::format("Incorrectly sized boot rom; expected {} bytes, given {} bytes. Reverting to HLE.",
					pif_rom.size(), size),
				UserMessage::Type::Warning
			);
			return false;
		}

		/* Read the file */
		ifs.seekg(0, ifs.beg);
		ifs.read((char*)pif_rom.data(), pif_rom.size());

		return true;
	}


	u8* GetPointerToRAM(const u32 offset)
	{
		return pif_ram.data() + (offset & (pif_ram.size() - 1)); /* Size is 0x40 bytes */
	}


	std::size_t GetNumberOfBytesUntilRAMEnd(const u32 offset)
	{
		return pif_ram.size() - (offset & (pif_ram.size() - 1)); /* Size is 0x40 bytes */
	}


	template<std::integral Int>
	Int ReadROM(const u32 offset)
	{
		return Memory::GenericRead<Int>(&pif_rom[offset]);
	}


	template<std::integral Int>
	Int ReadRAM(const u32 offset)
	{
		return Memory::GenericRead<Int>(&pif_ram[offset]);
	}


	template<std::size_t number_of_bytes>
	void WriteRAM(const u32 offset, const auto data)
	{
		Memory::GenericWrite<number_of_bytes>(&pif_ram[offset], data);
	}



	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadROM, const u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadRAM, const u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteRAM, const u32);
}