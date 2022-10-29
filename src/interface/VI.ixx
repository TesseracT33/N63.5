export module VI; /* Video Interface */

import Util;

import <bit>;
import <cassert>;
import <cstring>;

namespace VI
{
	export
	{
		enum Register {
			Ctrl, Origin, Width, VIntr, VCurrent, Burst,
			VSync, HSync, HSyncLeap, HVideo, VVideo,
			VBurst, XScale, YScale
		};

		struct Registers
		{
			s32 ctrl, origin, width, v_intr, v_current, burst, v_sync, h_sync, h_sync_leap;
			s32 h_video, v_video, v_burst, x_scale, y_scale, test_addr, stated_data;
		};

		void AddInitialEvents();
		void Initialize();
		const Registers& ReadAllRegisters();
		s32 ReadWord(u32 addr);
		void WriteWord(u32 addr, s32 data);
	}

	void CheckVideoInterrupt();
	void OnNewHalflineEvent();

	Registers vi;

	uint cpu_cycles_per_halfline;
	uint num_fields;
	uint num_halflines;
}