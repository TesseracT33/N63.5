export module VI; /* Video Interface */

import Util;

import <bit>;
import <cassert>;
import <concepts>;
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
		const Registers& ReadAllRegisters();
		void Initialize();
		template<std::integral Int> Int Read(u32 addr);
		template<size_t number_of_bytes> void Write(u32 addr, auto data);
	}

	void CheckVideoInterrupt();
	void OnNewHalflineEvent();

	Registers vi;

	uint cpu_cycles_per_halfline;
	uint num_fields;
	uint num_halflines;
}