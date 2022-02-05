module N64;

import Memory;
import MIPS_Interface;
import VR4300;

namespace N64
{
	bool PowerOn(
		const std::string& rom_path,
		SDL_Renderer* renderer,
		const unsigned window_width,
		const unsigned window_height)
	{
		bool success = Cartridge::LoadROM(rom_path);
		if (!success)
			return false;

		MIPS_Interface::Initialize();
		VR4300::PowerOn(true);

		return true;
	}

	void Run()
	{
		VR4300::Run(100);
	}
}