export module N64;

import <SDL.h>;

import <list>;
import <string>;

import NumericalTypes;

namespace N64
{
	export
	{
		enum class Event {
			PI_DMA_FINISH
		};

		static constexpr int cpu_cycles_per_second = 93'750'000;
		static constexpr int cpu_cycles_per_frame = cpu_cycles_per_second / 60; /* 1,562,500 */

		bool PowerOn(
			const std::string& rom_path,
			SDL_Renderer* renderer,
			const unsigned window_width,
			const unsigned window_height);

		void Run();

		void EnqueueEvent(Event event, int cycles_until_fire, int cycles_into_update);
	}

	void CheckEventQueue();
	void ExecuteEvent(Event event);

	struct EventItem
	{
		Event event;
		int time;
	};

	std::list<EventItem> event_queue{};

	int cpu_cycles_until_update_queue;
}