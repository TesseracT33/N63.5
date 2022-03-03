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