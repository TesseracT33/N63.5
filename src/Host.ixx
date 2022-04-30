export module Host;

import <bit>;

export namespace Host
{
	static_assert(std::endian::native == std::endian::little);

	constexpr bool has_avx2 = [] {
#ifdef __AVX2__
		return true;
#else
		return false;
#endif
	}();
}