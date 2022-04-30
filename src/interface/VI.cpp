module VI;

import HostSystem;
import Memory;
import MI;
import N64;
import RDRAM;
import Renderer;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace VI
{
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
		Renderer::SetFramebufferPtr(RDRAM::GetPointer(vi.origin));
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		const u32 offset = (addr >> 2) & 0xF;
		s32 ret;
		std::memcpy(&ret, (s32*)(&vi) + offset, 4);
		return Int(ret);
	}


	template<std::size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		const u32 offset = (addr >> 2) & 0xF;
		const auto word = s32(data);

		static constexpr u32 offset_ctrl = 0;
		static constexpr u32 offset_origin = 1;
		static constexpr u32 offset_width = 2;
		static constexpr u32 offset_v_intr = 3;
		static constexpr u32 offset_v_current = 4;
		static constexpr u32 offset_burst = 5;
		static constexpr u32 offset_v_sync = 6;
		static constexpr u32 offset_h_sync = 7;
		static constexpr u32 offset_h_sync_leap = 8;
		static constexpr u32 offset_h_video = 9;
		static constexpr u32 offset_v_video = 10;
		static constexpr u32 offset_v_burst = 11;
		static constexpr u32 offset_x_scale = 12;
		static constexpr u32 offset_y_scale = 13;
		static constexpr u32 offset_test_addr = 14;
		static constexpr u32 offset_stated_data = 15;

		switch (offset)
		{
		case offset_ctrl:
			vi.ctrl = word;
			/* Video pixel size */
			switch (vi.ctrl & 3)
			{
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

		case offset_origin:
			vi.origin = word;
			Renderer::SetFramebufferPtr(RDRAM::GetPointer(vi.origin));
			break;

		case offset_width:
			vi.width = word;
			Renderer::SetFramebufferWidth(vi.width);
			break;

		case offset_v_intr:
			vi.v_intr = word & 0x3FF; /* only bits 9-0 are writable. */
			CheckVideoInterrupt();
			break;

		case offset_v_current:
			vi.v_current = word & 0x3FF;
			MI::ClearInterruptFlag<MI::InterruptType::VI>();
			break;

		case offset_v_sync:
			vi.v_sync = word & 0x3FF;
			num_halflines = vi.v_sync >> 1;
			cpu_cycles_per_halfline = N64::cpu_cycles_per_frame / num_halflines; /* remainder being non-zero is taken caren of in the scheduler */
			break;

		default:
			break; /* TODO */
		}
	}


	void CheckVideoInterrupt()
	{
		if (vi.v_current == vi.v_intr)
		{
			MI::SetInterruptFlag<MI::InterruptType::VI>();
		}
	}


	void SetCurrentHalfline(const u32 halfline)
	{
		vi.v_current = halfline & 0x3FF;
		CheckVideoInterrupt();
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, u32)
}