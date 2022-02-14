export module VI; /* Video Interface */

import NumericalTypes;

import <array>;
import <concepts>;

namespace VI
{
	std::array<u8, 0x38> mem{};

	void WriteToControl0(const u8 data);

	void WriteToControl1(const u8 data);

	template<std::size_t start, std::size_t number_of_bytes>
	void WriteToOrigin(const auto shifted_data);

	export
	{
		template<std::integral Int>
		Int Read(const u32 addr);

		template<std::size_t number_of_bytes>
		void Write(const u32 addr, const auto data);
	}
}