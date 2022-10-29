module Cartridge;

import Memory;
import UserMessage;

import Util;

namespace Cartridge
{
	size_t GetNumberOfBytesUntilROMEnd(u32 addr)
	{
		u32 offset = (addr & 0x0FFF'FFFF) % rom.size();
		return rom.size() - offset;
	}


	u8* GetPointerToROM(u32 addr)
	{
		if (rom.size() == 0) {
			return nullptr;
		}
		u32 offset = (addr & 0x0FFF'FFFF) % rom.size();
		return rom.data() + offset;
	}


	u8* GetPointerToSRAM(u32 addr)
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


	template<std::signed_integral Int>
	Int ReadROM(u32 addr)
	{
		Int ret;
		std::memcpy(&ret, GetPointerToROM(addr), sizeof(Int));
		return std::byteswap(ret);
	}


	template<std::signed_integral Int>
	Int ReadSRAM(u32 addr)
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


	template<size_t num_bytes>
	void WriteSRAM(u32 addr, std::signed_integral auto data)
	{ /* CPU precondition: addr + number_of_bytes does not go beyond the next alignment boundary */
		if (sram.size() != 0) {
			data = std::byteswap(data);
			std::memcpy(GetPointerToSRAM(addr), &data, num_bytes);
		}
	}


	template s8 ReadROM<s8>(u32);
	template s16 ReadROM<s16>(u32);
	template s32 ReadROM<s32>(u32);
	template s64 ReadROM<s64>(u32);
	template s8 ReadSRAM<s8>(u32);
	template s16 ReadSRAM<s16>(u32);
	template s32 ReadSRAM<s32>(u32);
	template s64 ReadSRAM<s64>(u32);
	template void WriteSRAM<1>(u32, s8);
	template void WriteSRAM<1>(u32, s16);
	template void WriteSRAM<1>(u32, s32);
	template void WriteSRAM<1>(u32, s64);
	template void WriteSRAM<2>(u32, s16);
	template void WriteSRAM<2>(u32, s32);
	template void WriteSRAM<2>(u32, s64);
	template void WriteSRAM<3>(u32, s32);
	template void WriteSRAM<3>(u32, s64);
	template void WriteSRAM<4>(u32, s32);
	template void WriteSRAM<4>(u32, s64);
	template void WriteSRAM<5>(u32, s64);
	template void WriteSRAM<6>(u32, s64);
	template void WriteSRAM<7>(u32, s64);
	template void WriteSRAM<8>(u32, s64);
}