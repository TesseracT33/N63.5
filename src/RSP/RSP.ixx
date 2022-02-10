export module RSP;

import NumericalTypes;

import <array>;
import <concepts>;

namespace RSP
{
	std::array<u8, 0x1000> mem{}; /* Data & instruction memory */

	export
	{
		template<std::integral Int>
		Int ReadMemory(const u32 addr);

		template<std::size_t number_of_bytes>
		void WriteMemory(u32 addr, const auto data);
	}
}