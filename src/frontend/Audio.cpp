module;

#include "SDL.h"

module Audio;

import UserMessage;

void Audio::OnDeviceAdded(SDL_Event event)
{
	// TODO
}

void Audio::OnDeviceRemoved(SDL_Event event)
{
	// TODO
}

void Audio::Disable()
{
	// TODO
}

void Audio::Enable()
{
	// TODO
}

bool Audio::Init()
{
	if (SDL_Init(SDL_INIT_AUDIO) != 0) {
		UserMessage::Error(std::format("Failed to init audio: {}", SDL_GetError()));
		return false;
	}

	static constexpr int default_sample_rate = 44100;
	static constexpr int default_num_output_channels = 2;
	static constexpr int default_sample_buffer_size_per_channel = 512;

	SDL_AudioSpec desired_spec = {
		.freq = default_sample_rate,
		.format = AUDIO_S16MSB,
		.channels = default_num_output_channels,
		.samples = default_sample_buffer_size_per_channel
	};

	SDL_AudioSpec obtained_spec;
	audio_device_id = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, &obtained_spec, 0);
	if (audio_device_id == 0) {
		UserMessage::Warning(std::format("Could not open an audio device; {}", SDL_GetError()));
		return false;
	}

	//SetSampleRate(obtained_spec.freq);
	//num_output_channels = obtained_spec.channels;
	//sample_buffer_size_per_channel = obtained_spec.samples;

	SDL_PauseAudioDevice(audio_device_id, 0);

	return true;
}