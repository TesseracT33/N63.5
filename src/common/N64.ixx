export module N64;

import RDP;
import Util;

import "SDL.h";

import <iostream>;
import <optional>;
import <string>;

namespace N64
{
	export
	{
		enum class Control : u32 {
			A, B, CUp, CDown, CLeft, CRight, DUp, DDown, DLeft,
			DRight, JX, JY, ShoulderL, ShoulderR, Start, Z,
			CX, CY /* alternative to CUp, CDown etc for controlling C buttons using a joystick */
		};

		bool LoadBios(std::string const& bios_path);
		bool LoadGame(std::string const& game_path);
		bool LoadState();
		void Pause();
		bool PowerOn();
		void Reset();
		void Resume();
		void Run();
		bool SaveState();
		void Stop();
		void UpdateScreen();

		constexpr uint cpu_cycles_per_second = 93'750'000;
		constexpr uint rsp_cycles_per_second = 62'500'500;
		constexpr uint cpu_cycles_per_frame = cpu_cycles_per_second / 60; /* 1,562,500 */
		constexpr uint rsp_cycles_per_frame = rsp_cycles_per_second / 60; /* 1,041,675 */
	}

	bool bios_loaded;
	bool game_loaded;
}