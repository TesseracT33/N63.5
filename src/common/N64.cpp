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
		return PIF::LoadIPL12(path);
	}

	bool LoadGame(std::string const& path)
	{
		if (Cart::LoadRom(path)) {
			game_loaded = true;
		}
		else {
			game_loaded = false;
			UserMessage::Error(std::format("Failed to load rom at path {}", path));
		}
		return game_loaded;
	}

	bool LoadState()
	{
		return true;
	}

	void Pause()
	{

	}

	bool PowerOn()
	{
		AI::Initialize();
		MI::Initialize();
		PI::Initialize();
		RI::Initialize();
		SI::Initialize();
		VI::Initialize();
		RDRAM::Initialize();

		/* Power CPU after RSP, since CPU reads to RSP memory if hle_ipl and RSP clears it. */
		RSP::PowerOn();
		bool hle_ipl = !bios_loaded;
		VR4300::PowerOn(hle_ipl);

		RDP::Implementation rdp_impl = RDP::Implementation::ParallelRDP; // TODO: settable; load from user settings file
		if (!RDP::Initialize(rdp_impl)) {
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

	void Stop()
	{

	}

	void UpdateScreen()
	{
		RDP::implementation->UpdateScreen();
	}
}