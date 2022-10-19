export module N64;

import Util;

import <SDL.h>;

import <iostream>;
import <optional>;
import <string>;

namespace N64
{
	export
	{
		enum class Processor {
			CPU, RSP
		};

		constexpr uint cpu_cycles_per_second = 93'750'000;
		constexpr uint rsp_cycles_per_second = 62'500'500;
		constexpr uint cpu_cycles_per_frame = cpu_cycles_per_second / 60; /* 1,562,500 */
		constexpr uint rsp_cycles_per_frame = rsp_cycles_per_second / 60; /* 1,041,675 */

		bool PowerOn(
			const std::string& rom_path,
			const std::optional<std::string>& ipl_path,
			SDL_Renderer* renderer,
			uint window_width,
			uint window_height);
		void Run();
	}
}