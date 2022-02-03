export module Memory;

import <array>;
import <bit>;
import <cassert>;
import <concepts>;
import <functional>;

import NumericalTypes;

namespace Memory
{
	/* Use this function to do a conversion between little and big endian if necessary,
	   before the value is read back / written. */
	template<std::integral T>
	T convert_endian(const T value);

	template<std::integral T>
	T invalid_read(const u32 addr);

	template<std::integral T>
	void invalid_write(const u32 addr, const T data);

	export
	template<std::integral T>
	T ReadPhysical(const size_t number_of_bytes, const u32 physical_address);

	export
	template<std::integral T>
	void WritePhysical(const size_t number_of_bytes, const u32 physical_address, const T data);
}