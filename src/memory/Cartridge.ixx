export module Cartridge;

import NumericalTypes;

import <concepts>;
import <fstream>;
import <string>;
import <vector>;

namespace Cartridge
{
	export
	{
		bool LoadRom(const std::string& rom_path);

		template<std::integral T>
		T ReadRom(const std::size_t number_of_bytes, const u32 addr);

		template<std::integral T>
		T ReadSram(const std::size_t number_of_bytes, const u32 addr);
	}

	std::vector<u8> rom{};
}