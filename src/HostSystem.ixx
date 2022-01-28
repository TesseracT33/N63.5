export module HostSystem;

import <bit>;

export namespace HostSystem
{
	constexpr std::endian endianness = std::endian::native;
	static_assert(endianness == std::endian::little || endianness == std::endian::big);
}