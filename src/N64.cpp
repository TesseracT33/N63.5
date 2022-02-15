module N64;

import Cartridge;
import MI;
import Renderer;
import VI;
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

		MI::Initialize();
		VI::Initialize();
		VR4300::PowerOn(true);

		Renderer::SetRenderer(renderer);
		Renderer::SetWindowSize(window_width, window_height);

		return true;
	}


	void Run()
	{
		static constexpr int cycles_per_update = 1000;

		while (true)
		{
			VR4300::Run(cycles_per_update);
			Renderer::Render();
		}
	}


	void EnqueueEvent(Event event, int cycles_until_fire )
	{
		
	}
}