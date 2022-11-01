export module AI; /* Audio Interface */

import Util;

import <SDL.h>;

import <cstring>;
import <format>;

namespace AI
{
	export
	{
		void Initialize();
		s32 ReadReg(u32 addr);
		void WriteReg(u32 addr, s32 data);
	}

	void Sample();

	struct {
		u32 dram_addr, len, control, status, dacrate, bitrate, dummy0, dummy1;
	} ai{};

	struct {
		u32 frequency, period, precision;
	} dac{};

	u32 dma_length_buffer;
	u32 dma_address_buffer;
	uint dma_count;

	SDL_AudioDeviceID audio_device_id;
}