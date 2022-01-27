#define SDL_MAIN_HANDLED
#include "SDL.h"

#include <iostream>
#include <string>

import N64;
import UserMessage;

int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		exit(0);
	}

	const std::string rom_path = argv[1];

	SDL_SetMainReady();

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		std::cerr << SDL_GetError();
		exit(1);
	}

	/* Create SDL window and renderer */
	const std::string window_title = "N63.5 | " + rom_path;
	SDL_Window* sdl_window = SDL_CreateWindow(window_title.c_str(),
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 240,
		SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_FOCUS);

	if (sdl_window == nullptr)
	{
		std::cerr << SDL_GetError();
		exit(1);
	}
	UserMessage::SetWindow(sdl_window);

	SDL_Renderer* sdl_renderer = SDL_CreateRenderer(sdl_window, 0, SDL_RENDERER_ACCELERATED);
	if (sdl_renderer == nullptr)
	{
		UserMessage::Show(SDL_GetError(), UserMessage::Type::Error);
		exit(1);
	}

	bool success = N64::PowerOn(rom_path, sdl_renderer, 640, 480);
	if (!success)
	{
		UserMessage::Show("An error occured when starting the emulator.", UserMessage::Type::Error);
		exit(1);
	}
	N64::Run();

	SDL_DestroyRenderer(sdl_renderer);
	SDL_DestroyWindow(sdl_window);
	SDL_Quit();
}