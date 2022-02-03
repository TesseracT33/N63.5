module Cartridge;

import UserMessage;

namespace Cartridge
{
	bool load_rom(const std::string& rom_path)
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


	template<std::integral T>
	T read_rom(const std::size_t number_of_bytes, const u32 addr)
	{
		return 0;
	}


	template<std::integral T>
	T read_sram(const std::size_t number_of_bytes, const u32 addr)
	{
		return 0;
	}


	template u8 read_rom<u8>(const std::size_t number_of_byte, const u32 addr);
	template s8 read_rom<s8>(const std::size_t number_of_byte, const u32 addr);
	template u16 read_rom<u16>(const std::size_t number_of_byte, const u32 addr);
	template s16 read_rom<s16>(const std::size_t number_of_byte, const u32 addr);
	template u32 read_rom<u32>(const std::size_t number_of_byte, const u32 addr);
	template s32 read_rom<s32>(const std::size_t number_of_byte, const u32 addr);
	template u64 read_rom<u64>(const std::size_t number_of_byte, const u32 addr);
	template s64 read_rom<s64>(const std::size_t number_of_byte, const u32 addr);
	template u8 read_sram<u8>(const std::size_t number_of_byte, const u32 addr);
	template s8 read_sram<s8>(const std::size_t number_of_byte, const u32 addr);
	template u16 read_sram<u16>(const std::size_t number_of_byte, const u32 addr);
	template s16 read_sram<s16>(const std::size_t number_of_byte, const u32 addr);
	template u32 read_sram<u32>(const std::size_t number_of_byte, const u32 addr);
	template s32 read_sram<s32>(const std::size_t number_of_byte, const u32 addr);
	template u64 read_sram<u64>(const std::size_t number_of_byte, const u32 addr);
	template s64 read_sram<s64>(const std::size_t number_of_byte, const u32 addr);
}