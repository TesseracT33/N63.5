export module BitUtils;

import <concepts>;

export namespace BitUtils
{
	template<std::integral T> inline constexpr bool CheckBit(T num, unsigned pos)
	{
		return num & (1ull << pos);
	}

	template<std::integral T> inline constexpr bool CheckBit(T* num, unsigned pos)
	{
		return *num & (1ull << pos);
	}

	template<std::integral T> inline constexpr void SetBit(T& num, unsigned pos)
	{
		num |= (1ull << pos);
	}

	template<std::integral T> inline constexpr void SetBit(T* num, unsigned pos)
	{
		*num |= (1ull << pos);
	}

	template<std::integral T> inline constexpr void ClearBit(T& num, unsigned pos)
	{
		num &= ~(1ull << pos);
	}

	template<std::integral T> inline constexpr void ClearBit(T* num, unsigned pos)
	{
		*num &= ~(1ull << pos);
	}

	template<std::integral T> inline constexpr void ToggleBit(T& num, unsigned pos)
	{
		num ^= ~(1ull << pos);
	}

	template<std::integral T> inline constexpr void ToggleBit(T* num, unsigned pos)
	{
		*num ^= ~(1ull << pos);
	}
}