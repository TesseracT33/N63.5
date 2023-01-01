export module Input;

import N64;
import Util;

import "SDL.h";

import <format>;
import <iostream>;
import <limits>;
import <optional>;
import <unordered_map>;

namespace Input
{
	export {
		void ClearAllBindings();
		void ClearControllerBindings();
		void ClearKeyBindings();
		std::optional<u8> GetControllerBinding(N64::Control control);
		std::optional<SDL_Scancode> GetKeyBinding(N64::Control control);
		bool Init();
		bool LoadBindingsFromDisk();
		void OnControllerAxisMotion(SDL_Event const& event);
		void OnControllerButtonDown(SDL_Event const& event);
		void OnControllerButtonUp(SDL_Event const& event);
		void OnControllerDeviceAdded(SDL_Event const& event);
		void OnControllerDeviceRemoved(SDL_Event const& event);
		void OnKeyDown(SDL_Event const& event);
		void OnKeyUp(SDL_Event const& event);
		void OnMouseButtonDown(SDL_Event const& event);
		void OnMouseButtonUp(SDL_Event const& event);
		void RemoveControllerBinding(N64::Control control);
		void RemoveKeyBinding(N64::Control control);
		bool SaveBindingsToDisk();
		void SetBinding(SDL_GameControllerAxis axis, N64::Control control);
		void SetBinding(SDL_GameControllerButton button, N64::Control control);
		void SetBinding(SDL_Scancode key, N64::Control control);
		void SetDefaultBindings();
	}

	std::unordered_map<u8, N64::Control> controller_bindings; /* key: SDL_GameControllerAxis/SDL_GameControllerButton */
	std::unordered_map<SDL_Scancode, N64::Control> key_bindings;
}