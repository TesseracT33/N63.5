export module Util.SSE;

import NumericalTypes;

import <emmintrin.h>;
import <immintrin.h>;
import <smmintrin.h>;
import <tmmintrin.h>;

export
{
	__m128i _mm_byteswap_epi16(__m128i a);
	__m128i _mm_cmpge_epi16(__m128i a, __m128i b);
	__m128i _mm_cmple_epi16(__m128i a, __m128i b);
	__m128i _mm_cmplt_epu16(__m128i a, __m128i b);
	__m128i _mm_cmpneq_epi16(__m128i a, __m128i b);
	s16 _mm_getlane_epi16(const __m128i* num, int lane);
	u16 _mm_getlane_epu16(const __m128i* num, int lane);
	__m128i _mm_mulhi_epu16_epi16(__m128i a, __m128i b);
	__m128i _mm_nand_si128(__m128i a, __m128i b);
	__m128i _mm_nor_si128(__m128i a, __m128i b);
	__m128i _mm_not_si128(__m128i a);
	__m128i _mm_nxor_si128(__m128i a, __m128i b);
	void _mm_setlane_epi16(__m128i* num, int lane, s16 value);

	/* Note: not marking these as 'inline' could lead to improper initialization on MSVC. */
	inline const __m128i m128i_all_ones = _mm_set1_epi16(0xFFFF);
	inline const __m128i m128i_all_zeroes = _mm_set1_epi16(0);
	inline const __m128i m128i_epi16_sign_mask = _mm_set1_epi16(0x8000);
	inline const __m128i m128i_epi16_all_lanes_1 = _mm_set1_epi16(1);
	inline const __m128i x = _mm_set_epi64x(0, s64(-1)); /* todo: find good name... */
	inline const __m128i y = _mm_set_epi64x(s64(-1), 0);
}

const __m128i byteswap_epi16_mask = _mm_set_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);