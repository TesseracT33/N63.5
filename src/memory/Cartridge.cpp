module Cartridge;

import UserMessage;

namespace Cartridge
{
	bool LoadRom(const std::string& rom_path)
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
	T ReadRom(const std::size_t number_of_bytes, const u32 addr)
	{
		return 0;
	}


	template<std::integral T>
	T ReadSram(const std::size_t number_of_bytes, const u32 addr)
	{
		return 0;
	}


	template u8 ReadRom<u8>(const std::size_t number_of_bytes, const u32 addr);
	template s8 ReadRom<s8>(const std::size_t number_of_bytes, const u32 addr);
	template u16 ReadRom<u16>(const std::size_t number_of_bytes, const u32 addr);
	template s16 ReadRom<s16>(const std::size_t number_of_bytes, const u32 addr);
	template u32 ReadRom<u32>(const std::size_t number_of_bytes, const u32 addr);
	template s32 ReadRom<s32>(const std::size_t number_of_bytes, const u32 addr);
	template u64 ReadRom<u64>(const std::size_t number_of_bytes, const u32 addr);
	template s64 ReadRom<s64>(const std::size_t number_of_bytes, const u32 addr);
	template u8 ReadSram<u8>(const std::size_t number_of_bytes, const u32 addr);
	template s8 ReadSram<s8>(const std::size_t number_of_bytes, const u32 addr);
	template u16 ReadSram<u16>(const std::size_t number_of_bytes, const u32 addr);
	template s16 ReadSram<s16>(const std::size_t number_of_bytes, const u32 addr);
	template u32 ReadSram<u32>(const std::size_t number_of_bytes, const u32 addr);
	template s32 ReadSram<s32>(const std::size_t number_of_bytes, const u32 addr);
	template u64 ReadSram<u64>(const std::size_t number_of_bytes, const u32 addr);
	template s64 ReadSram<s64>(const std::size_t number_of_bytes, const u32 addr);
}