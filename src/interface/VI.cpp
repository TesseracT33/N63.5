module VI;

import MI;
import N64;
import RDP;
import Scheduler;

namespace VI
{
	void AddInitialEvents()
	{
		Scheduler::AddEvent(Scheduler::EventType::VINewHalfline, cpu_cycles_per_halfline, OnNewHalflineEvent);
	}


	void CheckVideoInterrupt()
	{
		bool prev_interrupt = interrupt;
		interrupt = vi.v_current == vi.v_intr;
		if (interrupt && !prev_interrupt) {
			MI::SetInterruptFlag(MI::InterruptType::VI);
		}
	}


	void Initialize()
	{
		std::memset(&vi, 0, sizeof(vi));
		vi.ctrl = 3;
		vi.width = 320;
		vi.v_intr = 0x3FF;
		vi.burst = 1;
		vi.v_sync = default_vsync_ntsc; /* todo: pal */
		vi.h_sync = 0x15'07FF;
		cpu_cycles_per_halfline = N64::cpu_cycles_per_frame / (vi.v_sync >> 1);
		Scheduler::ChangeEventTime(Scheduler::EventType::VINewHalfline, cpu_cycles_per_halfline);
	}


	bool Interlaced()
	{
		return vi.ctrl & 0x40;
	}


	void OnNewHalflineEvent()
	{
		vi.v_current += 2;
		if (vi.v_current >= vi.v_sync) {
			u32 field = vi.v_current & 1;
			vi.v_current = (field ^ 1) & u32(Interlaced());
			RDP::implementation->UpdateScreen();
		}
		CheckVideoInterrupt();
		Scheduler::AddEvent(Scheduler::EventType::VINewHalfline, cpu_cycles_per_halfline, OnNewHalflineEvent);
	}


	const Registers& ReadAllRegisters()
	{
		return vi;
	}


	s32 ReadReg(u32 addr)
	{
		static_assert(sizeof(vi) >> 2 == 0x10);
		u32 offset = addr >> 2 & 0xF;
		s32 ret;
		std::memcpy(&ret, (s32*)(&vi) + offset, 4);
		return ret;
	}


	void WriteReg(u32 addr, s32 data)
	{
		static_assert(sizeof(vi) >> 2 == 0x10);
		u32 offset = addr >> 2 & 0xF;

		switch (offset) {
		case Register::Ctrl:
			vi.ctrl = data;
			break;

		case Register::Origin:
			vi.origin = data & 0xFF'FFFF;
			break;

		case Register::Width:
			vi.width = data & 0xFFF;
			break;

		case Register::VIntr:
			vi.v_intr = data & 0x3FF;
			CheckVideoInterrupt();
			break;

		case Register::VCurrent:
			MI::ClearInterruptFlag(MI::InterruptType::VI);
			break;

		case Register::Burst:
			vi.burst = data & 0x3FFF'FFFF;
			break;

		case Register::VSync:
			vi.v_sync = data & 0x3FF ? data & 0x3FF : default_vsync_ntsc; /* todo: pal */
			cpu_cycles_per_halfline = N64::cpu_cycles_per_frame / (vi.v_sync >> 1);
			Scheduler::ChangeEventTime(Scheduler::EventType::VINewHalfline, cpu_cycles_per_halfline);
			break;

		case Register::HSync:
			vi.h_sync = data & 0x1F'0FFF;
			break;

		case Register::HSyncLeap:
			vi.h_sync_leap = data & 0xFFF'0FFF;
			break;

		case Register::HVideo:
			vi.h_video = data & 0x3FF'03FF;
			break;

		case Register::VVideo:
			vi.v_video = data & 0x3FF'03FF;
			break;

		case Register::VBurst:
			vi.v_burst = data & 0x3FF'03FF;
			break;

		case Register::XScale:
			vi.x_scale = data & 0xFFF'0FFF;
			break;

		case Register::YScale:
			vi.y_scale = data & 0xFFF'0FFF;
			break;

		case Register::TestAddr:
			vi.test_addr = data & 0x7F;
			break;

		case Register::StagedData:
			vi.staged_data = data;
			break;

		default:
			std::unreachable();
		}
	}
}