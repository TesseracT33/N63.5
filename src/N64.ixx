export module N64;

import <SDL.h>;

import <list>;
import <optional>;
import <string>;

import NumericalTypes;

namespace N64
{
	export
	{
		enum class Event {
			PiDmaFinish,
			SiDmaFinish,
			SpDmaFinish
		};

		enum class Processor {
			CPU, RSP
		};

		constexpr uint cpu_cycles_per_second = 93'750'000;
		constexpr uint rsp_cycles_per_second = 62'500'500;
		constexpr uint cpu_cycles_per_frame = cpu_cycles_per_second / 60; /* 1,562,500 */
		constexpr uint rsp_cycles_per_frame = rsp_cycles_per_second / 60; /* 1,041,675 */

		void EnqueueEvent(Event event, uint cycles_until_fire);
		bool PowerOn(
			const std::string& rom_path,
			const std::optional<std::string>& ipl_path,
			SDL_Renderer* renderer,
			uint window_width,
			uint window_height);
		void Run();
	}

	void CheckEventQueue();
	void ExecuteEvent(Event event);

	struct EventItem
	{
		Event event;
		int time;
	};

	uint cpu_cycles_until_update_queue;

	std::list<EventItem> event_queue{};
}