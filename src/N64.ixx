export module N64;

import <SDL.h>;

import <list>;
import <queue>;
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

		void EnqueueEvent(Event event, int cycles_until_fire);
	}
}