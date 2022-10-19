export module AI; /* Audio Interface */

import Util;

import <SDL.h>;

import <array>;
import <concepts>;
import <cstring>;
import <format>;

namespace AI
{
	export
	{
		void Initialize();
		template<std::integral Int> Int Read(u32 addr);
		template<size_t number_of_bytes> void Write(u32 addr, auto data);
	}

	void Sample();

	struct
	{
		u32 dram_addr, len, control, status, dacrate, bitrate;
	} ai{};

	struct
	{
		u32 frequency, period, precision;
	} dac{};

	u32 dma_length_buffer;
	u32 dma_address_buffer;
	uint dma_count;

	SDL_AudioDeviceID audio_device_id;
}