export module N64;

import <SDL.h>;

import Cartridge;

import <string>;

namespace N64
{
	export
	{
		bool PowerOn(
			const std::string& rom_path,
			SDL_Renderer* renderer,
			const unsigned window_width,
			const unsigned window_height);

		void Run();
	}

	Cartridge cartridge{};
}