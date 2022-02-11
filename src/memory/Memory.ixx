export module Memory;

import NumericalTypes;

import <array>;
import <bit>;
import <cassert>;
import <concepts>;
import <cstring>;

namespace Memory
{
	template<std::integral Int>
	Int InvalidRead(const u32 addr);

	template<std::integral Int>
	void InvalidWrite(const u32 addr, const Int data);

	export
	{
		template<std::integral Int>
		Int ReadPhysical(const u32 physical_address);

		template<std::size_t number_of_bytes>
		void WritePhysical(const u32 physical_address, const auto data);

		template<std::integral Int>
		Int GenericRead(const void* source);

		template<std::size_t number_of_bytes>
		void GenericWrite(void* destination, const auto data);

		template<std::integral Int>
		Int Byteswap(const Int value);
	}
}