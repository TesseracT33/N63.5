export module VI; /* Video Interface */

import Util;

import <bit>;
import <cstring>;
import <string_view>;
import <utility>;

namespace VI
{
	export
	{
		enum Register {
			Ctrl, Origin, Width, VIntr, VCurrent, Burst,
			VSync, HSync, HSyncLeap, HVideo, VVideo,
			VBurst, XScale, YScale, TestAddr, StagedData
		};

		struct Registers {
			u32 ctrl, origin, width, v_intr, v_current, burst, v_sync, h_sync, h_sync_leap,
				h_video, v_video, v_burst, x_scale, y_scale, test_addr, staged_data;
		};

		void AddInitialEvents();
		void Initialize();
		const Registers& ReadAllRegisters();
		s32 ReadReg(u32 addr);
		void WriteReg(u32 addr, s32 data);
	}

	void CheckVideoInterrupt();
	bool Interlaced();
	void OnNewHalflineEvent();
	constexpr std::string_view RegOffsetToStr(u32 reg_offset);

	constexpr u32 default_vsync_ntsc = 0x20D;

	Registers vi;

	bool interrupt;
	u32 cpu_cycles_per_halfline;
}