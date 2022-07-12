export module AI; /* Audio Interface */

import NumericalTypes;

import <SDL.h>;

import <array>;
import <concepts>;
import <cstring>;
import <format>;

namespace AI
{
	export
	{
		template<std::integral Int>
		Int Read(u32 addr);

		template<size_t number_of_bytes>
		void Write(u32 addr, auto data);

		void Initialize();
		void Step(uint cycles);
	}

	void Sample();

	struct
	{
		s32 dram_addr, len, control, status, dacrate, bitrate;
	} ai{};

	struct
	{
		s32 frequency, period, precision;
	} dac{};

	s32 dma_length_buffer;
	s32 dma_address_buffer;
	uint cycles;
	uint dma_count;

	SDL_AudioDeviceID audio_device_id;
}