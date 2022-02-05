module Memory;

import HostSystem;
import VR4300;

namespace Memory
{
	template<std::integral T>
	T ConvertEndian(const T value)
	{
		static_assert(sizeof T == 1 || sizeof T == 2 || sizeof T == 4 || sizeof T == 8);

		if constexpr (sizeof T == 1)
		{ /* No conversion necessary. N64 endianness irrelevant. */
			return value;
		}
		else
		{
			if (HostSystem::endianness == VR4300::endianness)
				return value;
			else
			{
				/* TODO: C++23 feature, to be used very soon */
				//return std::byteswap(value_to_byteswap);
				return value;
			}
		}
	}


	template<std::integral T>
	T InvalidRead(const u32 addr)
	{
		assert(false);
		return T();
	}


	template<std::integral T>
	void InvalidWrite(const u32 addr, const T data)
	{
		assert(false);
	}
}