module Memory;

namespace Memory
{
	/* All read functions in read_table return u64.
	   Use this function to truncate them to type T, and do conversion between little and big endian if necessary. */
	template<std::integral T>
	T TruncateAndConvertEndian(const u64 value)
	{
		static_assert(sizeof T == 1 || sizeof T == 2 || sizeof T == 4 || sizeof T == 8);

		if constexpr (sizeof T == 1) /* N64 endianness irrelevant */
		{
			if constexpr (HostSystem::endianness == std::endian::little)
				return T(value & 0xFF);
			else
				return T(value >> 56);
		}
		else
		{
			std::endian n64_endianness = std::endian::big; /* todo: determine n64 endianness */

			if (HostSystem::endianness == n64_endianness)
				return T(value);
			else
			{
				const T value_to_byteswap = [&] {
					if constexpr (HostSystem::endianness == std::endian::little)
					{
						if      constexpr (sizeof T == 2) return T(value & 0xFFFF);
						else if constexpr (sizeof T == 4) return T(value & 0xFFFF'FFFF);
						else                              return value; /* sizeof T == 8 */
					}
					else /* std::endian::big */
					{
						if      constexpr (sizeof T == 2) return T(value >> 48);
						else if constexpr (sizeof T == 4) return T(value >> 32);
						else                              return value; /* sizeof T == 8 */
					}
				}();

				/* TODO: C++23 feature, to be used very soon */
				//return std::byteswap(value_to_byteswap);
				return T(value);
			}
		}
	}

	template u8 TruncateAndConvertEndian<u8>(const u64 value);
	template s8 TruncateAndConvertEndian<s8>(const u64 value);
	template u16 TruncateAndConvertEndian<u16>(const u64 value);
	template s16 TruncateAndConvertEndian<s16>(const u64 value);
	template u32 TruncateAndConvertEndian<u32>(const u64 value);
	template s32 TruncateAndConvertEndian<s32>(const u64 value);
	template u64 TruncateAndConvertEndian<u64>(const u64 value);
	template s64 TruncateAndConvertEndian<s64>(const u64 value);
}