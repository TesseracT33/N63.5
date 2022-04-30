module Renderer;

import HostSystem;

namespace Renderer
{
	void Initialize(SDL_Renderer* renderer)
	{
		assert(renderer != nullptr);
		Renderer::renderer = renderer;
		RecreateTexture();
		rendering_is_enabled = false;
	}


	void Render()
	{
		if (!rendering_is_enabled)
			return;

		void* locked_pixels = nullptr;
		int locked_pixels_pitch = 0;
		SDL_LockTexture(texture, nullptr, &locked_pixels, &locked_pixels_pitch);

		SDL_ConvertPixels(
			framebuffer.width,        /* framebuffer width  */
			framebuffer.height,       /* framebuffer height */
			framebuffer.pixel_format, /* source format      */
			framebuffer.src_ptr,      /* source             */
			framebuffer.pitch,        /* source pitch       */
			framebuffer.pixel_format, /* destination format */
			locked_pixels,            /* destination        */
			framebuffer.pitch         /* destination pitch  */
		);

		SDL_UnlockTexture(texture);

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);
	}


	void RecreateTexture()
	{
		SDL_DestroyTexture(texture);
		texture = SDL_CreateTexture(
			renderer,
			framebuffer.pixel_format,
			SDL_TEXTUREACCESS_STREAMING,
			framebuffer.width,
			framebuffer.height
		);
	}


	template<PixelFormat pixel_format>
	void SetPixelFormat()
	{
		if constexpr (pixel_format == PixelFormat::Blank)
		{
			rendering_is_enabled = false;
		}
		else if constexpr (pixel_format == PixelFormat::RGBA5553)
		{
			/* Treat this as RGBA5551 */
			framebuffer.pixel_format = SDL_PIXELFORMAT_ABGR1555;
			framebuffer.bytes_per_pixel = 2;
			rendering_is_enabled = true;
		}
		else if constexpr (pixel_format == PixelFormat::RGBA8888)
		{
			framebuffer.pixel_format = SDL_PIXELFORMAT_ABGR8888;
			framebuffer.bytes_per_pixel = 4;
			rendering_is_enabled = true;
		}
		else
		{
			static_assert(pixel_format != pixel_format);
		}
		framebuffer.pitch = framebuffer.width * framebuffer.bytes_per_pixel;
	}


	void SetFramebufferPtr(u8* ptr)
	{
		framebuffer.src_ptr = ptr;
	}


	void SetFramebufferWidth(unsigned width)
	{
		assert(width > 0);
		framebuffer.width = width;
		framebuffer.height = width * 3 / 4; /* 4:3 aspect ratio */
		framebuffer.pitch = width * framebuffer.bytes_per_pixel;
		RecreateTexture();
	}


	void SetRenderer(SDL_Renderer* renderer)
	{
		Renderer::renderer = renderer;
	}


	void SetWindowSize(unsigned width, unsigned height)
	{
		assert(width > 0 && height > 0);
		SDL_RenderSetLogicalSize(renderer, width, height);
	}


	template void SetPixelFormat<PixelFormat::Blank>();
	template void SetPixelFormat<PixelFormat::RGBA5553>();
	template void SetPixelFormat<PixelFormat::RGBA8888>();
}