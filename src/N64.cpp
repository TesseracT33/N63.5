module N64;

import AI;
import Cartridge;
import Input;
import Memory;
import MI;
import PI;
import PIF;
import Renderer;
import RI;
import RSP;
import Scheduler;
import SI;
import VI;
import VR4300;

namespace N64
{
	bool PowerOn(
		const std::string& rom_path,
		const std::optional<std::string>& ipl_path,
		SDL_Renderer* renderer,
		uint window_width,
		uint window_height)
	{
		if (!Cartridge::LoadROM(rom_path)) {
			return false;
		}
		bool hle_ipl = true;
		if (ipl_path.has_value()) {
			hle_ipl = !PIF::LoadIPL12(ipl_path.value());
		}
		AI::Initialize();
		MI::Initialize();
		PI::Initialize();
		RI::Initialize();
		SI::Initialize();
		VI::Initialize();
		Memory::Initialize();
		/* Power CPU after RSP, since CPU reads to RSP memory if hle_ipl and RSP clears it. */
		RSP::PowerOn();
		VR4300::PowerOn(hle_ipl);
		Renderer::Initialize(renderer);
		Renderer::SetWindowSize(window_width, window_height);
		Scheduler::Initialize(); /* init last */
		return true;
	}


	void Run()
	{
		Scheduler::Run();
	}
}