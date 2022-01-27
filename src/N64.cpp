module N64;

import Memory;
import VR4300;

namespace N64
{
	bool PowerOn(
		const std::string& rom_path,
		SDL_Renderer* renderer,
		const unsigned window_width,
		const unsigned window_height)
	{
		bool success = cartridge.load_rom(rom_path);
		if (!success)
			return false;
		Memory::cartridge = &cartridge;

		VR4300::PowerOn(true);

		return true;
	}

	void Run()
	{
		VR4300::Run(100);
	}
}