module VI;

import HostSystem;
import Memory;
import MI;
import N64;
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
		Write<4>(VI_V_INTR, Memory::ByteswapOnLittleEndian<u32>(0x3FF));
		Write<4>(VI_BURST, Memory::ByteswapOnLittleEndian<u32>(1));
		Write<4>(VI_V_SYNC, Memory::ByteswapOnLittleEndian<u32>(0x20C));
		Write<4>(VI_H_SYNC, Memory::ByteswapOnLittleEndian<u32>(0x0015'07FF));

		/* NTSC "defaults" (?) */
		num_fields = 1;
		num_halflines = 262;
		cpu_cycles_per_halfline = N64::cpu_cycles_per_frame / num_halflines;
	}


	void ApplyWriteToControl()
	{
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

		/* Interlaced vs. progressive. Interlaced if bit 6 is set, otherwise progressive. */
		num_fields = mem[VI_CTRL + 3] & 0x40 ? 2 : 1;
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
			std::memcpy(&mem[offset], &word, 4);
			ApplyWriteToControl();
			break;

		case VI_ORIGIN:
		{
			std::memcpy(&mem[offset], &word, 4);
			u32 framebuffer_origin = Memory::ByteswapOnLittleEndian<u32>(word);
			Renderer::SetFramebufferPtr(RDRAM::GetPointer(framebuffer_origin));
			break;
		}

		case VI_WIDTH:
		{
			std::memcpy(&mem[offset], &word, 4);
			u32 framebuffer_width = Memory::ByteswapOnLittleEndian<u32>(word);
			Renderer::SetFramebufferWidth(framebuffer_width);
			break;
		}

		case VI_V_INTR:
			/* bits 9-0 are writable. */
			mem[offset + 3] = [&] {
				if constexpr (HostSystem::endianness == std::endian::big) return word       & 0xFF;
				else                                                      return word >> 24 & 0xFF;
			}();
			mem[offset + 2] = [&] {
				if constexpr (HostSystem::endianness == std::endian::big) return word >>  8 & 0x03;
				else                                                      return word >> 16 & 0x03;
			}();
			CheckVideoInterrupt();
			break;

		case VI_V_CURRENT:
			/* bits 9-0 are writable. */
			mem[offset + 3] = [&] {
				if constexpr (HostSystem::endianness == std::endian::big) return word & 0xFF;
				else                                                      return word >> 24 & 0xFF;
			}();
			mem[offset + 2] = [&] {
				if constexpr (HostSystem::endianness == std::endian::big) return word >> 8 & 0x03;
				else                                                      return word >> 16 & 0x03;
			}();
			MI::ClearInterruptFlag<MI::InterruptType::VI>();
			break;

		case VI_BURST:
			break;

		case VI_V_SYNC:
			/* bits 9-0 are writable. */
			mem[offset + 3] = [&] {
				if constexpr (HostSystem::endianness == std::endian::big) return word & 0xFF;
				else                                                      return word >> 24 & 0xFF;
			}();
			mem[offset + 2] = [&] {
				if constexpr (HostSystem::endianness == std::endian::big) return word >> 8 & 0x03;
				else                                                      return word >> 16 & 0x03;
			}();
			num_halflines = Memory::ByteswappedGenericRead<u32>(&mem[offset]) >> 1;
			cpu_cycles_per_halfline = N64::cpu_cycles_per_frame / num_halflines; /* remainder being non-zero is taken caren of in the scheduler */
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


	void CheckVideoInterrupt()
	{
		const u32 current_line = Memory::ByteswappedGenericRead<u32>(&mem[VI_V_CURRENT]);
		const u32 interrupt_line = Memory::ByteswappedGenericRead<u32>(&mem[VI_V_INTR]);
		if (current_line == interrupt_line)
		{
			MI::SetInterruptFlag<MI::InterruptType::VI>();
		}
	}


	void SetCurrentHalfline(u32 halfline)
	{
		halfline &= 0x3FF;
		Memory::ByteswappedGenericWrite<4>(&mem[VI_V_CURRENT], halfline);
		CheckVideoInterrupt();
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, const u32)
}