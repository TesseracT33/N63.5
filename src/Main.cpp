#define SDL_MAIN_HANDLED
#include "SDL.h"

import BuildOptions;
import Events;
import Logging;
import N64;
import RDP;

import <iostream>;
import <optional>;
import <string>;

int main(int argc, char* argv[])
{
	/* CLI arguments (beyond executable path):
	   1; path to rom (required)
	   2; path to IPL boot rom (optional)
	*/
	if (argc <= 1) {
		exit(0);
	}

	std::string rom_path = argv[1];
	std::optional<std::string> ipl_path;
	if (argc >= 3) {
		ipl_path.emplace(argv[2]);
	}

	SDL_SetMainReady();

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		std::cerr << SDL_GetError();
		exit(1);
	}
	if constexpr (enable_logging) {
		SetLogPath("F:\\n64.log");
	}
	if (!N64::PowerOn(rom_path, ipl_path, RDP::Implementation::None)) {
		std::cerr << "An error occured when starting the emulator.\n";
		exit(1);
	}

	Events::SetDefaultInputBindings();

	N64::Run();

	SDL_Quit();
}