module VI;

import Memory;
import MI;
import N64;
import RDRAM;
import RDP;
import Scheduler;

#include "../EnumerateTemplateSpecializations.h"

namespace VI
{
	void AddInitialEvents()
	{
		Scheduler::AddEvent(Scheduler::EventType::VINewHalfline, cpu_cycles_per_halfline, OnNewHalflineEvent);
	}


	void CheckVideoInterrupt()
	{
		if ((vi.v_current & 0x3FE) == vi.v_intr) {
			MI::SetInterruptFlag(MI::InterruptType::VI);
		}
	}


	void Initialize()
	{
		std::memset(&vi, 0, sizeof(vi));
		vi.v_intr = 0x3FF;
		vi.burst = 1;
		vi.v_sync = 0x20C;
		vi.h_sync = 0x15'07FF;

		/* NTSC "defaults" (?) */
		num_fields = 1;
		num_halflines = 262;
		cpu_cycles_per_halfline = N64::cpu_cycles_per_frame / num_halflines;
	}


	void OnNewHalflineEvent()
	{
		if (++vi.v_current == num_halflines) {
			vi.v_current = 0;
			RDP::implementation->UpdateScreen();
		}
		CheckVideoInterrupt();
		Scheduler::AddEvent(Scheduler::EventType::VINewHalfline, cpu_cycles_per_halfline, OnNewHalflineEvent);
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		u32 offset = (addr >> 2) & 0xF;
		s32 ret;
		std::memcpy(&ret, (s32*)(&vi) + offset, 4);
		return Int(ret);
	}


	const Registers& ReadAllRegisters()
	{
		return vi;
	}


	template<size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		u32 offset = (addr >> 2) & 0xF;
		auto word = s32(data);

		switch (offset) {
		case Register::Ctrl:
			vi.ctrl = word;
			/* Interlaced vs. progressive. Interlaced if bit 6 is set, otherwise progressive. */
			num_fields = 1 + bool(vi.ctrl & 0x40);
			break;

		case Register::Origin:
			vi.origin = word & 0x7F'FFFF;
			break;

		case Register::Width:
			vi.width = word & 0xFFF;
			break;

		case Register::VIntr:
			vi.v_intr = word & 0x3FF; /* only bits 9-0 are writable. */
			CheckVideoInterrupt();
			break;

		case Register::VCurrent:
			MI::ClearInterruptFlag(MI::InterruptType::VI);
			break;

		case Register::Burst:
			vi.burst = word & ~(3 << 30);
			break;

		case Register::VSync:
			vi.v_sync = word & 0x3FF;
			num_halflines = vi.v_sync >> 1;
			cpu_cycles_per_halfline = N64::cpu_cycles_per_frame / num_halflines;
			Scheduler::ChangeEventTime(Scheduler::EventType::VINewHalfline, cpu_cycles_per_halfline);
			break;

		case Register::HSync:
			vi.h_sync = word & 0x1F'0FFF;
			break;

		case Register::HSyncLeap:
			vi.h_sync_leap = word & 0x3FF'03FF;
			break;

		case Register::HVideo:
			vi.h_video = word & 0x3FF'03FF;
			break;

		case Register::VVideo:
			vi.v_video = word & 0x3FF'03FF;
			break;

		case Register::VBurst:
			vi.v_burst = word & 0x3FF'03FF;
			break;

		case Register::XScale:
			vi.x_scale = 0xFFF'0FFF;
			break;

		case Register::YScale:
			vi.y_scale = 0xFFF'0FFFF;
			break;

		default:
			assert(false);
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, u32)
}