module N64;

import AI;
import Cartridge;
import Input;
import Memory;
import MI;
import PI;
import PIF;
import RDP;
import RDRAM;
import RI;
import RSP;
import Scheduler;
import SI;
import UserMessage;
import VI;
import VR4300;

namespace N64
{
	bool PowerOn(
		const std::string& rom_path,
		const std::optional<std::string>& ipl_path,
		RDP::Implementation rdp_implementation)
	{
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
		RDRAM::Initialize();
		Memory::Initialize();

		if (!Cartridge::LoadRom(rom_path)) {
			return false;
		}

		/* Power CPU after RSP, since CPU reads to RSP memory if hle_ipl and RSP clears it. */
		RSP::PowerOn();
		VR4300::PowerOn(hle_ipl);

		if (!RDP::Initialize(rdp_implementation)) {
			return false;
		}
		SDL_Window* sdl_window = RDP::implementation->GetWindow();
		if (!sdl_window) {
			return false;
		}
		UserMessage::SetWindow(sdl_window);

		Scheduler::Initialize(); /* init last */

		return true;
	}


	void Run()
	{
		Scheduler::Run();
	}
}