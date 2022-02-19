module VI;

import Memory;
import RDRAM;
import Renderer;

#include "../Utils/EnumerateTemplateSpecializations.h"

#define VI_CTRL   0x00
#define VI_ORIGIN 0x04

namespace VI
{
	void Initialize()
	{
		mem.fill(0);
		Renderer::SetFramebufferPtr(RDRAM::GetPointer(0));
		WriteToControl0(0x03); /* TODO: default video size? Currently: 8/8/8/8 */
	}


	void WriteToControl0(const u8 data)
	{
		/* TODO */
		/* Video pixel size */
		switch (data & 0x03)
		{
		case 0b00: /* blank (no data and no sync, TV screens will either show static or nothing) */
			Renderer::SetColourFormat(0, 0, 0, 0);
			break;

		case 0b01: /* reserved */
			assert(false);
			break;

		case 0b10: /* 5/5/5/3 */
			Renderer::SetColourFormat(5, 5, 5, 3);
			break;

		case 0b11: /* 8/8/8/8 */
			Renderer::SetColourFormat(8, 8, 8, 8);
			break;
		}
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

		u32 origin;
		std::memcpy(&origin, &mem[VI_ORIGIN], 4);
		origin = Memory::Byteswap(origin);
		Renderer::SetFramebufferPtr(RDRAM::GetPointer(origin));
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		return Memory::GenericRead<Int>(&mem[addr & 0x3F]); /* TODO: number of register bytes is 0x38.. */
	}


	template<std::size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		switch (addr & 0x3F) /* TODO: number of register bytes is 0x38.. */
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