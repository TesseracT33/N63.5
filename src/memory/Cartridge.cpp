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


	//template<std::size_t number_of_bytes>
	//auto ReadRom(const u32 addr)
	//{
	//	return 0;
	//}


	//template<std::size_t number_of_bytes>
	//auto ReadSram(const u32 addr)
	//{
	//	return 0;
	//}


	//template u8 ReadRom<1>(const u32 addr);
	//template u16 ReadRom<2>(const u32 addr);
	//template u32 ReadRom<3>(const u32 addr);
	//template u32 ReadRom<4>(const u32 addr);
	//template u64 ReadRom<5>(const u32 addr);
	//template u64 ReadRom<6>(const u32 addr);
	//template u64 ReadRom<7>(const u32 addr);
	//template u64 ReadRom<8>(const u32 addr);
	//template u8 ReadSram<1>(const u32 addr);
	//template u16 ReadSram<2>(const u32 addr);
	//template u32 ReadSram<3>(const u32 addr);
	//template u32 ReadSram<4>(const u32 addr);
	//template u64 ReadSram<5>(const u32 addr);
	//template u64 ReadSram<6>(const u32 addr);
	//template u64 ReadSram<7>(const u32 addr);
	//template u64 ReadSram<8>(const u32 addr);
}