export module Renderer;

import Util;

import <SDL.h>;

import <bit>;
import <cassert>;
import <iostream>;

namespace Renderer
{
	export
	{
		enum class PixelFormat {
			Blank, RGBA5553, RGBA8888
		};

		template<PixelFormat pixel_format>
		void SetPixelFormat();

		void Initialize(SDL_Renderer* renderer = nullptr);
		void Render();
		void SetFramebufferPtr(u8* ptr);
		void SetFramebufferWidth(uint width);
		void SetRenderer(SDL_Renderer* renderer);
		void SetWindowSize(uint width, uint height);
	}

	void ByteswapFramebuffer();
	void RecreateTexture();

	struct Framebuffer
	{
		u8* src_ptr{};
		uint width = 320, height = 240, pitch = 320 * 4;
		uint bytes_per_pixel = 4;
		uint size = width * height * bytes_per_pixel;
		uint pixel_format = SDL_PIXELFORMAT_ABGR8888;
	} framebuffer{};

	bool rendering_is_enabled = false; /* set to false if VI.CTRL.TYPE != 0 */

	SDL_Renderer* renderer{};
	SDL_Texture* texture{};
}