export module Util.Numerics;

import NumericalTypes;

import <concepts>;

export
{
	/* Sign extends 'value' consisting of 'num_bits' bits to the width given by 'Int' */
	template<std::integral Int, int num_bits>
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
			/* Let UMAX be the number of bit width 'num_bits' with all bits set.
			* Let ~UMAX be one's complement of this number, but with bit width 'sizeof(Int) * 8'.
			* The below is then equivalent to ORing 'value' with ~UMAX when the sign bit of 'value' is set,
			* and otherwise ORing with 0.
			*
			* Example: value = 0x7FFF, Int = s16, num_bits = 15.
			* We OR 'value' with 0x1000 to get 0xFFFF.
			* In code: OR 'value' with 0x1000 * (value >> (num_bits - 1) & 1) => 0x1000 * (0x7FFF >> 14 & 1) <=> 0x1000 * 1 => 0x1000
			* However, there is a shortcut: the first factor 0x1000 can instead be 2, i.e., 0x1000 / (1 << (num_bits - 1)),
			* if the second factor instead is (value >> (num_bits - 1) & 1) * (1 << (num_bits - 1)), i.e., value & 1 << (num_bits - 1).
			* Hypothetically, this saves on a single left-shift operation at run-time. Godbolt shows it to produce better code.
			*/
			static constexpr Int mask = (Int(-1) << num_bits) / (Int(1) << (num_bits - 1));
			return Int(value) | (mask * (value & 1 << (num_bits - 1)));
		}
	}
}