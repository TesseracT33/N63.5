export module Renderer;

import HostSystem;
import NumericalTypes;

import <SDL.h>;

import <bit>;
import <cassert>;

namespace Renderer
{
	SDL_Renderer* renderer{};
	SDL_Texture* texture{};

	struct Framebuffer
	{
		u8* src_ptr{};
		int width = 320, height = 240, pitch = 320 * 4;
		int bytes_per_pixel = 4;
		uint pixel_format = [] {
			if constexpr (HostSystem::endianness == std::endian::little)
				return SDL_PIXELFORMAT_ABGR8888;
			else return SDL_PIXELFORMAT_RGBA8888;
		}();
	} framebuffer{};

	void RecreateTexture();

	export
	{
		enum class PixelFormat { Blank, RGBA5553, RGBA8888 };

		template<PixelFormat pixel_format>
		void SetPixelFormat();

		void Initialize(SDL_Renderer* renderer = nullptr);
		void Render();
		void SetFramebufferPtr(u8* ptr);
		void SetFramebufferWidth(unsigned width);
		void SetRenderer(SDL_Renderer* renderer);
		void SetWindowSize(unsigned width, unsigned height);
	}
}