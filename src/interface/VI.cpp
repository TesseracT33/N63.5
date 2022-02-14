module VI;

import Memory;

#include "../Utils/EnumerateTemplateSpecializations.h"

#define VI_CTRL   0x00
#define VI_ORIGIN 0x04

namespace VI
{
	void WriteToControl0(const u8 data)
	{
		/* TODO */
	}


	void WriteToControl1(const u8 data)
	{
		/* TODO */
	}


	template<std::size_t start, std::size_t number_of_bytes /* how many bytes we want to write */>
	void WriteToOrigin(const auto shifted_data)
	{
		constexpr std::size_t number_of_bytes_to_write = /* how many bytes we will actually write (we cannot go past the end of this memory region) */
			number_of_bytes <= 3 - start ? number_of_bytes : 3 - start;
		Memory::GenericWrite<number_of_bytes_to_write>(&mem[VI_ORIGIN + start], shifted_data);
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		return Memory::GenericRead<Int>(&mem[addr % mem.size()]);
	}


	template<std::size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		switch (addr % mem.size())
		{
		case VI_CTRL:
			WriteToControl0(u8(data));
			if constexpr (number_of_bytes >= 2)
				WriteToControl1(u8(data >> 8));
			if constexpr (number_of_bytes >= 5)
				WriteToOrigin<0, number_of_bytes - 4>(data >> 32);
			break;

		case VI_CTRL + 1:
			WriteToControl1(u8(data));
			if constexpr (number_of_bytes >= 4)
				WriteToOrigin<0, number_of_bytes - 3>(data >> 24);
			break;

		case VI_CTRL + 2:
			if constexpr (number_of_bytes >= 3)
				WriteToOrigin<0, number_of_bytes - 2>(data >> 16);
			break;

		case VI_CTRL + 3:
			if constexpr (number_of_bytes >= 2)
				WriteToOrigin<0, number_of_bytes - 1>(data >> 8);
			break;

		case VI_ORIGIN:
			WriteToOrigin<0, number_of_bytes>(data);
			break;

		case VI_ORIGIN + 1:
			WriteToOrigin<1, number_of_bytes>(data);
			break;

		case VI_ORIGIN + 2:
			WriteToOrigin<2, number_of_bytes>(data);
			break;

		default: /* TODO */
			break;
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, const u32)
}