export module Cartridge;

import MemoryUtils;
import NumericalTypes;

import <fstream>;
import <string>;
import <vector>;

namespace Cartridge
{
	export
	{
		bool LoadRom(const std::string& rom_path);

		template<std::size_t number_of_bytes>
		auto ReadRom(const u32 addr)
		{
			return MemoryUtils::ConstructUnsignedIntegral<number_of_bytes>(0);
		}

		template<std::size_t number_of_bytes>
		auto ReadSram(const u32 addr)
		{
			return MemoryUtils::ConstructUnsignedIntegral<number_of_bytes>(0);
		}
	}

	std::vector<u8> rom{};
}