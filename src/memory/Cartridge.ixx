export module Cartridge;

import NumericalTypes;

import <fstream>;
import <string>;
import <vector>;

namespace Cartridge
{
	export
	{
		bool load_rom(const std::string& rom_path);
		u64 read(const u32 addr);
	}

	std::vector<u8> rom{};
}