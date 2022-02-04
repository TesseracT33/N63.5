export module MemoryUtils;

import MemoryAccess;
import NumericalTypes;

import <concepts>;
import <cstring>;

export namespace MemoryUtils
{
	/* The result will different from sizeof(T) only for unaligned memory accesses. */
	template<std::integral T, MemoryAccess::Alignment alignment>
	constexpr std::size_t GetNumberOfBytesToAccess(const u32 addr)
	{
		if constexpr (alignment == MemoryAccess::Alignment::Aligned)
			return sizeof T;
		else
			return sizeof T - (addr & (sizeof T - 1));
	}

	template<std::integral T>
	T GenericRead(const std::size_t number_of_bytes, const void* source)
	{
		T ret;
		std::memcpy(&ret, source, number_of_bytes);
		return ret;
	}

	template<std::integral T>
	void GenericWrite(const std::size_t number_of_bytes, void* destination, const T data)
	{
		std::memcpy(destination, &data, number_of_bytes);
	}
}