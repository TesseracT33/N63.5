export module Renderer;

import NumericalTypes;

import <SDL.h>;

import <cassert>;

namespace Renderer
{
	u8* framebuffer_ptr{};
	SDL_Renderer* renderer{};

	export
	{
		void Render();
		void SetFramebufferPtr(u8* ptr);
		void SetRenderer(SDL_Renderer* renderer);
		void SetWindowSize(int width, int height);
	}
}