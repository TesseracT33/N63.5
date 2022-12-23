module N64;

import AI;
import Cart;
import Events;
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
	bool LoadBios(std::string const& path)
	{
		return true;
	}

	bool LoadState()
	{
		return true;
	}

	void Pause()
	{

	}

	bool PowerOn(
		RDP::Implementation rdp_implementation,
		std::string const& rom_path,
		std::optional<std::string> const& ipl_path)
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

		if (rom_path.empty()) {
			game_loaded = false;
		}
		else if (Cart::LoadRom(rom_path)) {
			game_loaded = true;
		}
		else {
			game_loaded = false;
			UserMessage::ShowError(std::format("Failed to load rom at path {}", rom_path));
		}

		/* Power CPU after RSP, since CPU reads to RSP memory if hle_ipl and RSP clears it. */
		RSP::PowerOn();
		VR4300::PowerOn(hle_ipl);

		if (!RDP::Initialize(rdp_implementation)) {
			return false;
		}

		Scheduler::Initialize(); /* init last */

		return true;
	}

	void Reset()
	{

	}

	void Resume()
	{

	}

	void Run()
	{
		Scheduler::Run();
	}

	bool SaveState()
	{
		return true;
	}

	bool StartGame(std::string const& rom_path)
	{
		return true;
	}

	void Stop()
	{

	}

	void UpdateScreen()
	{
		RDP::implementation->UpdateScreen();
	}
}