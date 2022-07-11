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
		enum class Event
		{
			PI_DMA_FINISH,
			SI_DMA_FINISH,
			SP_DMA_FINISH
		};

		enum class Processor
		{
			CPU,
			RSP
		};

		constexpr int cpu_cycles_per_second = 93'750'000;
		constexpr int rsp_cycles_per_second = 62'500'500;
		constexpr int cpu_cycles_per_frame = cpu_cycles_per_second / 60; /* 1,562,500 */
		constexpr int rsp_cycles_per_frame = rsp_cycles_per_second / 60; /* 1,041,675 */

		bool PowerOn(
			const std::string& rom_path,
			const std::optional<std::string>& ipl_path,
			SDL_Renderer* renderer,
			uint window_width,
			uint window_height);

		void EnqueueEvent(Event event, uint cycles_until_fire, uint cycles_into_update);
		void Run();
	}

	void CheckEventQueue();
	void ExecuteEvent(Event event);

	struct EventItem
	{
		Event event;
		int time;
	};

	std::list<EventItem> event_queue{};

	uint cpu_cycles_until_update_queue;
}