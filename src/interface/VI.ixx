export module VI; /* Video Interface */

import NumericalTypes;

import <array>;
import <cassert>;
import <concepts>;

namespace VI
{
	std::array<u8, 0x38> mem{};

	void ApplyWriteToControl();

	export
	{
		void Initialize();

		template<std::integral Int>
		Int Read(const u32 addr);

		template<std::size_t number_of_bytes>
		void Write(const u32 addr, const auto data);

		void CheckVideoInterrupt();
		void SetCurrentHalfline(u32 halfline);

		int num_fields;
		int num_halflines;
		int cpu_cycles_per_halfline;
	}
}