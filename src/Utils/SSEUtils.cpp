module SSEUtils;


const __m128i m128i_all_zeroes = _mm_set1_epi16(0);
const __m128i m128i_all_ones = _mm_set1_epi16(0xFFFF);


__m128i _mm_cmpge_epi16(__m128i a, __m128i b)
{
	return _mm_or_si128(_mm_cmpgt_epi16(a, b), _mm_cmpeq_epi16(a, b));
}


__m128i _mm_cmple_epi16(__m128i a, __m128i b)
{
	return _mm_or_si128(_mm_cmplt_epi16(a, b), _mm_cmpeq_epi16(a, b));
}


__m128i _mm_cmplt_epu16(const __m128i a, const __m128i b)
{
	static const __m128i add = _mm_set1_epi16(0x8000);
	return _mm_cmplt_epi16(_mm_add_epi16(a, add), _mm_add_epi16(b, add));
}


__m128i _mm_cmpneq_epi16(const __m128i a, const __m128i b)
{
	return _mm_not_si128(_mm_cmpeq_epi16(a, b));
}


__m128i _mm_mulhi_epu16_epi16(const __m128i a, const __m128i b)
{
	/* High 16 bits of unsigned x signed multiplication is equivalent to high 16 bits of unsigned multiplication,
	   but where we have subtracted the first number if the second one is negative.
	   Source: https://stackoverflow.com/questions/28807341/simd-signed-with-unsigned-multiplication-for-64-bit-64-bit-to-128-bit */
	const __m128i unsigned_prod = _mm_mulhi_epu16(a, b);
	const __m128i correction = _mm_mullo_epi16(a, _mm_srli_epi16(_mm_cmplt_epi16(b, m128i_all_zeroes), 15));
	return _mm_sub_epi16(unsigned_prod, correction);
}


__m128i _mm_nand_si128(const __m128i a, const __m128i b)
{
	return _mm_not_si128(_mm_and_si128(a, b));
}


__m128i _mm_nor_si128(const __m128i a, const __m128i b)
{
	return _mm_not_si128(_mm_or_si128(a, b));
}


__m128i _mm_not_si128(const __m128i a)
{
	return _mm_xor_si128(a, m128i_all_ones);
}


__m128i _mm_nxor_si128(const __m128i a, const __m128i b)
{
	return _mm_not_si128(_mm_nxor_si128(a, b));
}


void _mm_addlane_epi16(__m128i* num, int lane, s16 value)
{
	*((s16*)num + lane) += value;
}


s16 _mm_getlane_epi16(const __m128i* num, int lane)
{
	return *((s16*)num + lane);
}


u16 _mm_getlane_epu16(const __m128i* num, int lane)
{
	return *((u16*)num + lane);
}


void _mm_setlane_epi16(__m128i* num, int lane, s16 value)
{
	*((s16*)num + lane) = value;
}
