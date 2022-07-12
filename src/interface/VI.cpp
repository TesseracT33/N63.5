module VI;

import Memory;
import MI;
import N64;
import RDRAM;
import Renderer;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace VI
{
	void CheckVideoInterrupt()
	{
		if ((vi.v_current & 0x3FE) == vi.v_intr) {
			MI::SetInterruptFlag(MI::InterruptType::VI);
		}
	}


	void Initialize()
	{
		vi.ctrl = 0;
		vi.origin = 0;
		vi.v_intr = 0x3FF;
		vi.burst = 1;
		vi.v_sync = 0x20C;
		vi.h_sync = 0x15'07FF;

		/* NTSC "defaults" (?) */
		num_fields = 1;
		num_halflines = 262;
		cpu_cycles_per_halfline = N64::cpu_cycles_per_frame / num_halflines;
		Renderer::SetFramebufferPtr(RDRAM::GetPointerToMemory(vi.origin));
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		u32 offset = (addr >> 2) & 0xF;
		s32 ret;
		std::memcpy(&ret, (s32*)(&vi) + offset, 4);
		return Int(ret);
	}


	void SetCurrentHalfline(const u32 halfline)
	{
		vi.v_current = halfline & 0x3FF;
		CheckVideoInterrupt();
	}


	template<size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		u32 offset = (addr >> 2) & 0xF;
		auto word = s32(data);

		enum RegOffset {
			Ctrl, Origin, Width, VIntr, VCurrent, Burst,
			VSync, HSync, HSyncLeap, HVideo, VVideo,
			VBurst, XScale, YScale, TestAddr, StatedData
		};

		switch (offset) {
		case RegOffset::Ctrl:
			vi.ctrl = word;
			/* Video pixel size */
			switch (vi.ctrl & 3) {
			case 0b00: /* blank (no data and no sync, TV screens will either show static or nothing) */
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
			num_fields = 1 + bool(vi.ctrl & 0x40);
			break;

		case RegOffset::Origin:
			vi.origin = word & 0x7F'FFFF;
			Renderer::SetFramebufferPtr(RDRAM::GetPointerToMemory(vi.origin));
			break;

		case RegOffset::Width:
			vi.width = word & 0xFFF;
			Renderer::SetFramebufferWidth(vi.width);
			break;

		case RegOffset::VIntr:
			vi.v_intr = word & 0x3FF; /* only bits 9-0 are writable. */
			CheckVideoInterrupt();
			break;

		case RegOffset::VCurrent:
			vi.v_current = word & 0x3FF;
			MI::ClearInterruptFlag(MI::InterruptType::VI);
			break;

		case RegOffset::VSync:
			vi.v_sync = word & 0x3FF;
			num_halflines = vi.v_sync >> 1;
			cpu_cycles_per_halfline = N64::cpu_cycles_per_frame / num_halflines; /* remainder being non-zero is taken caren of in the scheduler */
			break;

		default:
			break; /* TODO */
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, u32)
}