export module Cartridge;

import MemoryUtils;
import NumericalTypes;

import <cassert>;
import <fstream>;
import <string>;
import <vector>;

namespace Cartridge
{
	export
	{
		bool LoadROM(const std::string& rom_path);

		template<std::size_t number_of_bytes>
		auto ReadROM(const u32 addr)
		{
			return MemoryUtils::ConstructUnsignedIntegral<number_of_bytes>(0);
		}

		template<std::size_t number_of_bytes>
		auto ReadSRAM(const u32 addr)
		{
			return MemoryUtils::ConstructUnsignedIntegral<number_of_bytes>(0);
		}

		template<std::size_t number_of_bytes>
		void WriteROM(const u32 addr, const auto data)
		{
			assert(false);
		}

		template<std::size_t number_of_bytes>
		void WriteSRAM(const u32 addr, const auto data)
		{

		}
	}

	std::vector<u8> rom{};
	std::vector<u8> ram{};
}