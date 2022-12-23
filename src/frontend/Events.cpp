module;

#include "imgui_impl_sdl.h"

module Events;

import Gui;
import PIF;

namespace Events
{
	bool LoadInputBindingsFromDisk()
	{
		return false; // TODO
	}

	void OnControllerAxisMotion()
	{
		// TODO: possibly store and compare against previous axis value to stop from going further for every single axis change.
		if (auto binding_it = controller_bindings.find(event.caxis.axis); binding_it != controller_bindings.end()) {
			using enum N64::Control;
			N64::Control n64_control = binding_it->second;
			if (n64_control == JX || n64_control == JY) {
				PIF::OnJoystickMovement(n64_control, event.caxis.value);
			}
			else if (n64_control == CX || n64_control == CY) {
				// Register a C button press for a "large enough" (absolute) axis value.
				static constexpr s16 axis_min_threshold = std::numeric_limits<s16>::min() * 3 / 4;
				static constexpr s16 axis_max_threshold = std::numeric_limits<s16>::max() * 3 / 4;
				if (event.caxis.value < axis_min_threshold) {
					PIF::OnButtonDown(n64_control == CX ? CLeft : CUp);
				}
				else if (event.caxis.value > axis_max_threshold) {
					PIF::OnButtonDown(n64_control == CX ? CRight : CDown);
				}
				else {
					PIF::OnButtonUp(n64_control == CX ? CLeft : CUp);
					PIF::OnButtonUp(n64_control == CX ? CRight : CDown);
				}
			}
			else {
				std::unreachable();
			}
		}
	}

	void OnControllerButtonDown()
	{
		if (auto binding_it = controller_bindings.find(event.cbutton.button); binding_it != controller_bindings.end()) {
			N64::Control n64_control = binding_it->second;
			PIF::OnButtonDown(n64_control);
		}
	}

	void OnControllerButtonUp()
	{
		if (auto binding_it = controller_bindings.find(event.cbutton.button); binding_it != controller_bindings.end()) {
			N64::Control n64_control = binding_it->second;
			PIF::OnButtonUp(n64_control);
		}
	}

	void OnControllerDeviceAdded()
	{
		// TODO
	}

	void OnControllerDeviceRemoved()
	{
		// TODO
	}

	void OnKeyDown()
	{
		SDL_Keycode keycode = event.key.keysym.sym;
		if ((SDL_GetModState() & SDL_Keymod::KMOD_CTRL) != 0 && keycode != SDLK_LCTRL && keycode != SDLK_RCTRL) { /* LCTRL/RCTRL is held */
			Gui::OnCtrlKeyPress(keycode);
		}
		else if (auto binding_it = key_bindings.find(event.key.keysym.scancode); binding_it != key_bindings.end()) {
			N64::Control n64_control = binding_it->second;
			PIF::OnButtonDown(n64_control);
		}
	}

	void OnKeyUp()
	{
		if (auto binding_it = key_bindings.find(event.key.keysym.scancode); binding_it != key_bindings.end()) {
			N64::Control n64_control = binding_it->second;
			PIF::OnButtonUp(n64_control);
		}
	}

	void OnMouseButtonDown()
	{
		// TODO
	}

	void OnMouseButtonUp()
	{
		// TODO
	}

	void OnSdlQuit()
	{
		SDL_Quit();
		exit(0);
	}

	void Poll()
	{
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			switch (event.type) {
			case SDL_CONTROLLERAXISMOTION   : OnControllerAxisMotion();    break;
			case SDL_CONTROLLERBUTTONDOWN   : OnControllerButtonDown();    break;
			case SDL_CONTROLLERBUTTONUP     : OnControllerButtonUp();      break;
			case SDL_CONTROLLERDEVICEADDED  : OnControllerDeviceAdded();   break;
			case SDL_CONTROLLERDEVICEREMOVED: OnControllerDeviceRemoved(); break;
			case SDL_KEYDOWN                : OnKeyDown();                 break;
			case SDL_KEYUP                  : OnKeyUp();                   break;
			case SDL_MOUSEBUTTONDOWN        : OnMouseButtonDown();         break;
			case SDL_MOUSEBUTTONUP          : OnMouseButtonUp();           break;
			case SDL_QUIT                   : OnSdlQuit();                 break;
			}
		}
	}

	bool SaveInputBindingsToDisk()
	{
		return false; // TODO
	}

	void SetDefaultInputBindings()
	{
		controller_bindings.clear();
		controller_bindings[SDL_CONTROLLER_BUTTON_A] = N64::Control::A;
		controller_bindings[SDL_CONTROLLER_BUTTON_B] = N64::Control::B;
		controller_bindings[SDL_CONTROLLER_BUTTON_START] = N64::Control::Start;
		controller_bindings[SDL_CONTROLLER_BUTTON_DPAD_UP] = N64::Control::DUp;
		controller_bindings[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = N64::Control::DDown;
		controller_bindings[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = N64::Control::DLeft;
		controller_bindings[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = N64::Control::DRight;
		controller_bindings[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] = N64::Control::ShoulderL;
		controller_bindings[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = N64::Control::ShoulderR;
		controller_bindings[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] = N64::Control::Z;
		controller_bindings[SDL_CONTROLLER_AXIS_RIGHTX] = N64::Control::CX;
		controller_bindings[SDL_CONTROLLER_AXIS_RIGHTY] = N64::Control::CY;
		controller_bindings[SDL_CONTROLLER_AXIS_LEFTX] = N64::Control::JX;
		controller_bindings[SDL_CONTROLLER_AXIS_LEFTY] = N64::Control::JY;

		// Basically only meant for testing
		key_bindings.clear();
		key_bindings[SDL_SCANCODE_A] = N64::Control::A;
		key_bindings[SDL_SCANCODE_B] = N64::Control::B;
		key_bindings[SDL_SCANCODE_RETURN] = N64::Control::Start;
		key_bindings[SDL_SCANCODE_UP] = N64::Control::DUp;
		key_bindings[SDL_SCANCODE_DOWN] = N64::Control::DDown;
		key_bindings[SDL_SCANCODE_LEFT] = N64::Control::DLeft;
		key_bindings[SDL_SCANCODE_RIGHT] = N64::Control::DRight;
		key_bindings[SDL_SCANCODE_L] = N64::Control::ShoulderL;
		key_bindings[SDL_SCANCODE_R] = N64::Control::ShoulderR;
		key_bindings[SDL_SCANCODE_Z] = N64::Control::Z;
		key_bindings[SDL_SCANCODE_KP_8] = N64::Control::CUp;
		key_bindings[SDL_SCANCODE_KP_2] = N64::Control::CDown;
		key_bindings[SDL_SCANCODE_KP_4] = N64::Control::CLeft;
		key_bindings[SDL_SCANCODE_KP_6] = N64::Control::CRight;
	}
}