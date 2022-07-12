export module BitUtils;

import <concepts>;

export namespace BitUtils
{
	template<std::integral T> constexpr bool GetBit(T num, int pos)
	{
		return num & (1ull << pos);
	}

	template<std::integral T> constexpr bool GetBit(T* num, int pos)
	{
		return *num & (1ull << pos);
	}

	template<std::integral T> constexpr void SetBit(T& num, int pos)
	{
		num |= (1ull << pos);
	}

	template<std::integral T> constexpr void SetBit(T* num, int pos)
	{
		*num |= (1ull << pos);
	}

	template<std::integral T> constexpr void ClearBit(T& num, int pos)
	{
		num &= ~(1ull << pos);
	}

	template<std::integral T> constexpr void ClearBit(T* num, int pos)
	{
		*num &= ~(1ull << pos);
	}

	template<std::integral T> constexpr void ToggleBit(T& num, int pos)
	{
		num ^= ~(1ull << pos);
	}

	template<std::integral T> constexpr void ToggleBit(T* num, int pos)
	{
		*num ^= ~(1ull << pos);
	}
}