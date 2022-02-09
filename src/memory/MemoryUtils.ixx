export module MemoryUtils;

import MemoryAccess;
import NumericalTypes;

import <concepts>;
import <cstring>;

export namespace MemoryUtils
{
	/* The result will different from sizeof(T) only for unaligned memory accesses. */
	template<std::integral T, MemoryAccess::Alignment alignment>
	constexpr std::size_t GetNumberOfBytesToAccess(const auto addr)
	{
		if constexpr (alignment == MemoryAccess::Alignment::Aligned)
			return sizeof T;
		else
			return sizeof T - (addr & (sizeof T - 1));
	}

	template<std::size_t number_of_bytes>
	constexpr auto ConstructUnsignedIntegral(const auto data)
	{
		     if constexpr (number_of_bytes == 1) return u8(data);
		else if constexpr (number_of_bytes == 2) return u16(data);
		else if constexpr (number_of_bytes <= 4) return u32(data);
		else if constexpr (number_of_bytes <= 8) return u64(data);
		else                                     static_assert(false);
	}

	template<std::size_t number_of_bytes>
	constexpr auto ConstructSignedIntegral(const auto data)
	{
		     if constexpr (number_of_bytes == 1) return s8(data);
		else if constexpr (number_of_bytes == 2) return s16(data);
		else if constexpr (number_of_bytes <= 4) return s32(data);
		else if constexpr (number_of_bytes <= 8) return s64(data);
		else                                     static_assert(false);
	}
}