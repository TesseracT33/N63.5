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
		void CheckVideoInterrupt();
		void Initialize();
		void SetCurrentHalfline(u32 halfline);

		template<std::integral Int>
		Int Read(u32 addr);

		template<size_t number_of_bytes>
		void Write(u32 addr, auto data);

		uint cpu_cycles_per_halfline;
		uint num_fields;
		uint num_halflines;
	}

	struct
	{
		s32 ctrl, origin, width, v_intr, v_current, burst, v_sync, h_sync, h_sync_leap;
		s32 h_video, v_video, v_burst, x_scale, y_scale, test_addr, stated_data;
	} vi{};
}