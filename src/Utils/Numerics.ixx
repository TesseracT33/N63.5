export module Util.Numerics;

import NumericalTypes;

import <concepts>;
import <type_traits>;

export
{
	/* Sign extends 'value' consisting of 'num_bits' bits to the width given by 'Int' */
	template<std::integral Int, uint num_bits>
	constexpr Int SignExtend(auto value)
	{
		static_assert(num_bits > 0);
		static_assert(sizeof(Int) * 8 >= num_bits);
		static_assert(sizeof(Int) <= 8);

		if constexpr (num_bits == 8) {
			return Int(s8(value));
		}
		else if constexpr (num_bits == 16) {
			return Int(s16(value));
		}
		else if constexpr (num_bits == 32) {
			return Int(s32(value));
		}
		else if constexpr (num_bits == 64) {
			return value;
		}
		else {
			using sInt = std::make_signed<Int>::type;
			constexpr auto shift_amount = 8 * sizeof(Int) - num_bits;
			auto signed_int = static_cast<sInt>(value);
			return static_cast<Int>(static_cast<sInt>(signed_int << shift_amount) >> shift_amount);
		}
	}
}