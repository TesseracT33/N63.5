export module Events;

import N64;
import Util;

import "SDL.h";

import <limits>;
import <unordered_map>;

namespace Events
{
	export bool LoadInputBindingsFromDisk();
	export void Poll();
	export bool SaveInputBindingsToDisk();
	export void SetDefaultInputBindings();

	void OnControllerAxisMotion();
	void OnControllerButtonDown();
	void OnControllerButtonUp();
	void OnControllerDeviceAdded();
	void OnControllerDeviceRemoved();
	void OnKeyDown();
	void OnKeyUp();
	void OnMouseButtonDown();
	void OnMouseButtonUp();
	void OnSdlQuit();

	SDL_Event event;

	std::unordered_map<u8, N64::Control> controller_bindings; /* key: SDL_GameControllerAxis/SDL_GameControllerButton */
	std::unordered_map<SDL_Scancode, N64::Control> key_bindings;
}