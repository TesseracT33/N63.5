export module PIF;

import NumericalTypes;

import <array>;
import <string>;

namespace PIF
{
	std::array<u8, 1> pif_rom{};
	std::array<u8, 1> pif_ram{};

	export
	{
		void HLE_IPL() {};
		void LLE_IPL() {};

		bool LoadIPL123(const std::string& path) { return false; };

		template<std::size_t number_of_bytes>
		auto ReadROM(const u32 addr)
		{

		}

		template<std::size_t number_of_bytes>
		auto ReadRAM(const u32 addr)
		{

		}

		template<std::size_t number_of_bytes>
		void WriteROM(const u32 addr, const auto data)
		{

		}

		template<std::size_t number_of_bytes>
		void WriteRAM(const u32 addr, const auto data)
		{

		}
	}
}