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
		mem[VI_CTRL + 3] |= 0x03; /* TODO: default video size? Currently: 8/8/8/8 */
		ApplyWriteToControl();
	}


	void ApplyWriteToControl()
	{
		/* TODO */
		/* Video pixel size */
		switch (mem[VI_CTRL + 3] & 0x03)
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


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		return Memory::GenericRead<Int>(&mem[addr & 0x3F]); /* TODO: number of register bytes is 0x38.. */
	}


	template<std::size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		/* TODO: for now, only allow word-aligned writes. Force 'data' to be a 32-bit integer. */
		const u32 offset = addr & 0x3C; /* TODO: number of register bytes is 0x38.. */
		const u32 word = static_cast<u32>(data);
		switch (offset)
		{
		case VI_CTRL:
			std::memcpy(&mem[VI_CTRL], &word, 4);
			ApplyWriteToControl();
			break;

		case VI_ORIGIN:
		{
			std::memcpy(&mem[VI_ORIGIN], &word, 4);
			u32 framebuffer_origin = Memory::ByteswapOnLittleEndian<u32>(word);
			Renderer::SetFramebufferPtr(RDRAM::GetPointer(framebuffer_origin));
			break;
		}
			

		default: /* TODO */
			break;
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, const u32)
}