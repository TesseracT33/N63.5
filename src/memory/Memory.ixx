export module Memory;

import HostSystem;
import VR4300;
import NumericalTypes;

import <array>;
import <bit>;
import <cassert>;
import <concepts>;
import <cstring>;

namespace Memory
{
	template<std::integral Int>
	Int InvalidRead(const u32 addr);

	template<std::integral Int>
	void InvalidWrite(const u32 addr, const Int data);

	export
	{
		/* Use this function to do a conversion between little and big endian if necessary,
		   before the value is read back / written. */
		template<std::integral Int>
		constexpr Int ByteswapOnLittleEndian(const Int value)
		{
			if constexpr (sizeof Int == 1 || HostSystem::endianness == VR4300::endianness)
			{ /* No conversion necessary. */
				return value;
			}
			else
			{
				return std::byteswap(value);
			}
		}

		template<std::integral Int>
		Int ReadPhysical(const u32 physical_address);

		template<std::size_t number_of_bytes>
		void WritePhysical(const u32 physical_address, const auto data);

		template<std::integral Int>
		Int GenericRead(const void* source);

		template<std::size_t number_of_bytes>
		void GenericWrite(void* destination, const auto data);

		template<std::integral Int>
		Int ByteswappedGenericRead(const void* source);
	}
}