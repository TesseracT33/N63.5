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
		bool load_rom(const std::string& rom_path);

		template<std::integral T>
		T read_rom(const std::size_t number_of_bytes, const u32 addr);

		template<std::integral T>
		T read_sram(const std::size_t number_of_bytes, const u32 addr);
	}

	std::vector<u8> rom{};
}