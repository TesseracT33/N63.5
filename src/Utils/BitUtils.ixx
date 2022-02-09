export module BitUtils;

import <concepts>;

export namespace BitUtils
{
	template<std::integral T> inline constexpr bool CheckBit(T num, const unsigned pos)
	{
		return num & (1ull << pos);
	}

	template<std::integral T> inline constexpr bool CheckBit(T* num, const unsigned pos)
	{
		return *num & (1ull << pos);
	}

	template<std::integral T> inline constexpr void SetBit(T& num, const unsigned pos)
	{
		num |= (1ull << pos);
	}

	template<std::integral T> inline constexpr void SetBit(T* num, const unsigned pos)
	{
		*num |= (1ull << pos);
	}

	template<std::integral T> inline constexpr void ClearBit(T& num, const unsigned pos)
	{
		num &= ~(1ull << pos);
	}

	template<std::integral T> inline constexpr void ClearBit(T* num, const unsigned pos)
	{
		*num &= ~(1ull << pos);
	}

	template<std::integral T> inline constexpr void ToggleBit(T& num, const unsigned pos)
	{
		num ^= ~(1ull << pos);
	}

	template<std::integral T> inline constexpr void ToggleBit(T* num, const unsigned pos)
	{
		*num ^= ~(1ull << pos);
	}
}