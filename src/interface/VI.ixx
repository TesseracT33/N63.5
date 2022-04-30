export module VI; /* Video Interface */

import NumericalTypes;

import <bit>;
import <cassert>;
import <concepts>;
import <cstring>;

namespace VI
{
	struct
	{
		s32 ctrl, origin, width, v_intr, v_current, burst, v_sync, h_sync, h_sync_leap;
		s32 h_video, v_video, v_burst, x_scale, y_scale, test_addr, stated_data;
	} vi{};

	export
	{
		void Initialize();

		template<std::integral Int>
		Int Read(u32 addr);

		template<std::size_t number_of_bytes>
		void Write(u32 addr, auto data);

		void CheckVideoInterrupt();
		void SetCurrentHalfline(u32 halfline);

		int num_fields;
		int num_halflines;
		int cpu_cycles_per_halfline;
	}
}