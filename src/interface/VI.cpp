module VI;

import Memory;
import RDRAM;
import Renderer;

#include "../Utils/EnumerateTemplateSpecializations.h"

#define VI_CTRL        0x00
#define VI_ORIGIN      0x04
#define VI_WIDTH       0x08
#define VI_V_INTR      0x0C
#define VI_V_CURRENT   0x10
#define VI_BURST       0x14
#define VI_V_SYNC      0x18
#define VI_H_SYNC      0x1C
#define VI_H_SYNC_LEAP 0x20
#define VI_H_VIDEO     0x24
#define VI_V_VIDEO     0x28
#define VI_V_BURST     0x2C
#define VI_X_SCALE     0x30
#define VI_Y_SCALE     0x34
#define VI_TEST_ADDR   0x38
#define VI_STAGED_DATA 0x3C

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
			Renderer::SetPixelFormat<Renderer::PixelFormat::Blank>();
			break;

		case 0b01: /* reserved */
			Renderer::SetPixelFormat<Renderer::PixelFormat::Blank>();
			break;

		case 0b10: /* 5/5/5/3 */
			Renderer::SetPixelFormat<Renderer::PixelFormat::RGBA5553>();
			break;

		case 0b11: /* 8/8/8/8 */
			Renderer::SetPixelFormat<Renderer::PixelFormat::RGBA8888>();
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
		std::memcpy(&mem[offset], &word, 4);

		switch (offset)
		{
		case VI_CTRL:
			ApplyWriteToControl();
			break;

		case VI_ORIGIN:
		{
			u32 framebuffer_origin = Memory::ByteswapOnLittleEndian<u32>(word);
			Renderer::SetFramebufferPtr(RDRAM::GetPointer(framebuffer_origin));
			break;
		}

		case VI_WIDTH:
		{
			u32 framebuffer_width = Memory::ByteswapOnLittleEndian<u32>(word);
			Renderer::SetFramebufferWidth(framebuffer_width);
			break;
		}

		case VI_V_INTR:
			break;

		case VI_V_CURRENT:
			break;

		case VI_BURST:
			break;

		case VI_V_SYNC:
			break;

		case VI_H_SYNC:
			break;

		case VI_H_SYNC_LEAP:
			break;

		case VI_H_VIDEO:
			break;

		case VI_V_VIDEO:
			break;

		case VI_V_BURST:
			break;

		case VI_X_SCALE:
			break;

		case VI_Y_SCALE:
			break;

		case VI_TEST_ADDR:
			break;

		case VI_STAGED_DATA:
			break;
			
		default:
			break;
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, const u32)
}