export module AI; /* Audio Interface */

import NumericalTypes;

import <SDL.h>;

import <array>;
import <concepts>;
import <cstring>;
import <format>;

namespace AI
{
	struct
	{
		s32 dram_addr, len, control, status, dacrate, bitrate;
	} ai{};

	int dma_count;
	s32 dma_length_buffer;
	s32 dma_address_buffer;
	int cycles;

	SDL_AudioDeviceID audio_device_id;

	struct
	{
		s32 frequency;
		s32 period;
		s32 precision;
	} dac{};

	void Sample();

	export
	{
		void Initialize();

		void Step(int cycles);

		template<std::integral Int>
		Int Read(u32 addr);

		template<std::size_t number_of_bytes>
		void Write(u32 addr, auto data);
	}
}