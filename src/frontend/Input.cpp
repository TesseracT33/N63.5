module Input;

import Gui;
import N64;
import UserMessage;

void Input::ClearAllBindings()
{
	ClearControllerBindings();
	ClearKeyBindings();
}

void Input::ClearControllerBindings()
{
	controller_bindings.clear();
}

void Input::ClearKeyBindings()
{
	key_bindings.clear();
}

std::optional<u8> Input::GetControllerBinding(N64::Control control)
{
	for (auto const& [sdl_button, n64_control] : controller_bindings) {
		if (n64_control == control) {
			return sdl_button;
		}
	}
	return {};
}

std::optional<SDL_Scancode> Input::GetKeyBinding(N64::Control control)
{
	for (auto const& [sdl_key, n64_control] : key_bindings) {
		if (n64_control == control) {
			return sdl_key;
		}
	}
	return {};
}

bool Input::Init()
{
	if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
		std::cerr << std::format("Failed to init gamecontroller system: {}\n", SDL_GetError());
		return false;
	}
	if (!LoadBindingsFromDisk()) {
		UserMessage::Error("Failed to load user bindings! Using defaults.");
		SetDefaultBindings();
		SaveBindingsToDisk();
	}
	return true;
}

bool Input::LoadBindingsFromDisk()
{
	return true; // TODO
}

void Input::OnControllerAxisMotion(SDL_Event const& event)
{
	// TODO: possibly store and compare against previous axis value to stop from going further for every single axis change.
	if (auto binding_it = controller_bindings.find(event.caxis.axis); binding_it != controller_bindings.end()) {
		using enum N64::Control;
		N64::Control n64_control = binding_it->second;
		s16 axis_value = event.caxis.value;
		if (n64_control == JX || n64_control == JY) {
			N64::OnJoystickMovement(n64_control, axis_value);
		}
		else if (n64_control == CX || n64_control == CY) {
			// Register a C button press for a "large enough" (absolute) axis value.
			static constexpr s16 axis_min_threshold = std::numeric_limits<s16>::min() * 3 / 4;
			static constexpr s16 axis_max_threshold = std::numeric_limits<s16>::max() * 3 / 4;
			if (axis_value < axis_min_threshold) {
				N64::OnButtonDown(n64_control == CX ? CLeft : CUp);
			}
			else if (axis_value > axis_max_threshold) {
				N64::OnButtonDown(n64_control == CX ? CRight : CDown);
			}
			else {
				N64::OnButtonUp(n64_control == CX ? CLeft : CUp);
				N64::OnButtonUp(n64_control == CX ? CRight : CDown);
			}
		}
		else {
			std::unreachable();
		}
	}
}

void Input::OnControllerButtonDown(SDL_Event const& event)
{
	if (auto binding_it = controller_bindings.find(event.cbutton.button); binding_it != controller_bindings.end()) {
		N64::Control n64_control = binding_it->second;
		N64::OnButtonDown(n64_control);
	}
}

void Input::OnControllerButtonUp(SDL_Event const& event)
{
	if (auto binding_it = controller_bindings.find(event.cbutton.button); binding_it != controller_bindings.end()) {
		N64::Control n64_control = binding_it->second;
		N64::OnButtonUp(n64_control);
	}
}

void Input::OnControllerDeviceAdded(SDL_Event const& event)
{
	// TODO
}

void Input::OnControllerDeviceRemoved(SDL_Event const& event)
{
	// TODO
}

void Input::OnKeyDown(SDL_Event const& event)
{
	SDL_Keycode keycode = event.key.keysym.sym;
	if ((SDL_GetModState() & SDL_Keymod::KMOD_CTRL) != 0 && keycode != SDLK_LCTRL && keycode != SDLK_RCTRL) { /* LCTRL/RCTRL is held */
		Gui::OnCtrlKeyPress(keycode);
	}
	else if (auto binding_it = key_bindings.find(event.key.keysym.scancode); binding_it != key_bindings.end()) {
		N64::Control n64_control = binding_it->second;
		N64::OnButtonDown(n64_control);
	}
}

void Input::OnKeyUp(SDL_Event const& event)
{
	if (auto binding_it = key_bindings.find(event.key.keysym.scancode); binding_it != key_bindings.end()) {
		N64::Control n64_control = binding_it->second;
		N64::OnButtonUp(n64_control);
	}
}

void Input::OnMouseButtonDown(SDL_Event const& event)
{
	// TODO
}

void Input::OnMouseButtonUp(SDL_Event const& event)
{
	// TODO
}

void Input::RemoveControllerBinding(N64::Control control)
{
	for (auto it = controller_bindings.begin(); it != controller_bindings.end(); ++it) {
		if (it->second == control) {
			controller_bindings.erase(it);
			break;
		}
	}
}

void Input::RemoveKeyBinding(N64::Control control)
{
	for (auto it = key_bindings.begin(); it != key_bindings.end(); ++it) {
		if (it->second == control) {
			key_bindings.erase(it);
			break;
		}
	}
}

bool Input::SaveBindingsToDisk()
{
	return false; // TODO
}

void Input::SetBinding(SDL_GameControllerAxis axis, N64::Control control)
{
	RemoveControllerBinding(control);
	controller_bindings[axis] = control;
}

void Input::SetBinding(SDL_GameControllerButton button, N64::Control control)
{
	RemoveControllerBinding(control);
	controller_bindings[button] = control;
}

void Input::SetBinding(SDL_Scancode key, N64::Control control)
{
	RemoveKeyBinding(control);
	key_bindings[key] = control;
}

void Input::SetDefaultBindings()
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