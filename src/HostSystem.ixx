export module HostSystem;

import <bit>;

export namespace HostSystem
{
	constexpr std::endian endianness = std::endian::native;
	static_assert(endianness == std::endian::little || endianness == std::endian::big);

	constexpr bool has_avx2 = [] {
#ifdef __AVX2__
		return true;
#else
		return false;
#endif
	}();
}