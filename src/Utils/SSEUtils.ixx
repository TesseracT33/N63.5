export module SSEUtils;

import NumericalTypes;

import <emmintrin.h>;
import <immintrin.h>;
import <smmintrin.h>;
import <tmmintrin.h>;

export
{
	__m128i _mm_cmpge_epi16(__m128i a, __m128i b);
	__m128i _mm_cmple_epi16(__m128i a, __m128i b);
	__m128i _mm_cmplt_epu16(__m128i a, __m128i b);
	__m128i _mm_cmpneq_epi16(__m128i a, __m128i b);
	__m128i _mm_mulhi_epu16_epi16(__m128i a, __m128i b);
	__m128i _mm_nand_si128(__m128i a, __m128i b);
	__m128i _mm_nor_si128(__m128i a, __m128i b);
	__m128i _mm_not_si128(__m128i a);
	__m128i _mm_nxor_si128(__m128i a, __m128i b);

	void _mm_addlane_epi16(__m128i* num, int lane, s16 value);
	s16 _mm_getlane_epi16(const __m128i* num, int lane);
	u16 _mm_getlane_epu16(const __m128i* num, int lane);
	void _mm_setlane_epi16(__m128i* num, int lane, s16 value);
}