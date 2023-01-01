export module Audio;

import "SDL.h";

import <format>;

namespace Audio
{
	export
	{
		void OnDeviceAdded(SDL_Event event);
		void OnDeviceRemoved(SDL_Event event);
		void Disable();
		void Enable();
		bool Init();
	}

	SDL_AudioDeviceID audio_device_id;
}