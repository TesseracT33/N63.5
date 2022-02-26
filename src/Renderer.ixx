export module Renderer;

import NumericalTypes;

import <SDL.h>;

import <bit>;
import <cassert>;

namespace Renderer
{
	u8* framebuffer_ptr{};
	SDL_Renderer* renderer{};

	s32 maskR{}, maskG{}, maskB{}, maskA{};

	export
	{
		void Initialize(SDL_Renderer* renderer = nullptr);
		void Render();
		void SetColourFormat(unsigned num_red_bits, unsigned num_green_bits, unsigned num_blue_bits, unsigned num_alpha_bits);
		void SetFramebufferPtr(u8* ptr);
		void SetRenderer(SDL_Renderer* renderer);
		void SetWindowSize(int width, int height);
	}
}