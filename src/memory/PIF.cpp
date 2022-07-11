module PIF;

import Memory;
import UserMessage;

import Util.Files;

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
		const auto& rom = optional_rom.value();
		std::memcpy(memory.data(), rom.data(), rom_size);
		return true;
	}


	u8* GetPointerToRAM(const u32 address)
	{
		return memory.data() + rom_size + (address & (ram_size - 1)); /* Size is 0x40 bytes */
	}


	u8* GetPointerToMemory(const u32 address)
	{
		return memory.data() + (address & 0x7FF);
	}


	std::size_t GetNumberOfBytesUntilRAMEnd(const u32 offset)
	{
		return ram_size - (offset & (ram_size - 1)); /* Size is 0x40 bytes */
	}


	template<std::integral Int>
	Int ReadMemory(const u32 addr)
	{ /* CPU precondition: addr is aligned */
		Int ret;
		std::memcpy(&ret, memory.data() + (addr & 0x7FF), sizeof(Int));
		return std::byteswap(ret);
	}


	template<std::size_t number_of_bytes>
	void WriteMemory(u32 addr, auto data)
	{ /* CPU precondition: write does not go to the next boundary */
		addr &= 0x7FF;
		if (addr >= rom_size) /* $0-$7BF: rom; $7C0-$7FF: ram $*/
		{
			data = std::byteswap(data);
			std::memcpy(memory.data() + addr, &data, number_of_bytes);
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadMemory, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteMemory, u32);
}