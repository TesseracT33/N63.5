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


	u64 read(const u32 addr)
	{
		return u64();
	}
}