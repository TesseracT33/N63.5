import Gui;
import Logging;
import N64;
import RDP;

import <iostream>;
import <optional>;
import <string>;

int main(int argc, char* argv[])
{
	/* CLI arguments (beyond executable path):
	   1; path to rom (optional)
	   2; path to IPL boot rom (optional)
	*/
	std::optional<std::string> rom_path, ipl_path;
	if (argc > 1) {
		rom_path = argv[1];
	}
	if (argc > 2) {
		ipl_path = argv[2];
	}

	if (!InitializeLogging()) {
		std::cerr << "[warning] Failed to initialize logging.\n";
	}

	if (!Gui::Init()) {
		std::cerr << "[fatal] Failed to initialize GUI.\n";
		exit(1);
	}

	if (!N64::Init()) {
		std::cerr << "[fatal] An error occured when starting the emulator.\n";
		exit(1);
	}

	bool start_game_immediately = false;
	if (rom_path.has_value()) {
		if (N64::LoadGame(rom_path.value())) {
			start_game_immediately = true;
		}
		else {
			std::cerr << "[error] Failed to load rom at path " << rom_path.value() << '\n';
		}
	}

	if (ipl_path.has_value() && !N64::LoadBios(ipl_path.value())) {
		std::cerr << "[error] Failed to load bios at path " << ipl_path.value() << '\n';
	}

	Gui::Run(start_game_immediately);

	Gui::TearDown();
}