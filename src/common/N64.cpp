module N64;

import AI;
import BuildOptions;
import Cart;
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
	bool Init()
	{
		AI::Initialize();
		MI::Initialize();
		PI::Initialize();
		RI::Initialize();
		SI::Initialize();
		VI::Initialize();
		RDRAM::Initialize();

		VR4300::PowerOn();
		RSP::PowerOn();
		RDP::Initialize();

		Scheduler::Initialize(); /* init last */

		return true;
	}

	bool LoadBios(std::filesystem::path const& path)
	{
		return PIF::LoadIPL12(path);
	}

	bool LoadGame(std::filesystem::path const& path)
	{
		if (Cart::LoadRom(path)) {
			game_loaded = true;
		}
		else {
			game_loaded = false;
			UserMessage::Error(std::format("Failed to load rom at path {}", path.string()));
		}
		return game_loaded;
	}

	bool LoadState()
	{
		return true; // TODO
	}

	void OnButtonDown(Control control)
	{
		PIF::OnButtonAction<true>(control);
	}

	void OnButtonUp(Control control)
	{
		PIF::OnButtonAction<false>(control);
	}

	void OnJoystickMovement(Control control, s16 axis_value)
	{
		PIF::OnJoystickMovement(control, axis_value);
	}

	void Pause()
	{
		// TODO
	}

	void Reset()
	{
		// TODO
	}

	void Resume()
	{
		 // TODO
	}

	void Run()
	{
		if (!running) {
			Reset();
			bool hle_pif = !bios_loaded || skip_boot_rom;
			VR4300::InitRun(hle_pif);
			running = true;
		}
		Scheduler::Run();
	}

	bool SaveState()
	{
		return true; // TODO
	}

	void Stop()
	{
		Scheduler::Stop();
		running = false;
	}

	void UpdateScreen()
	{
		RDP::implementation->UpdateScreen();
	}
}