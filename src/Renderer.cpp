module Renderer;

import VI;
import Util;

import <bit>;

namespace Renderer
{
	void ByteswapFramebuffer()
	{
		/* RDRAM is stored in big endian. If using RGBA5551 (RGBA5553), colour channels
		   will go across byte boundaries. Thus, it is not enough to use SDL_PIXELFORMAT_ABGR1555;
		   we need to byteswap each halfword in the framebuffer before copying it into the texture. */
		for (uint i = 0; i < framebuffer.size; i += 2) {
			u8* framebuffer_src = (u8*)(framebuffer.src_ptr) + i;
			u8 tmp = *framebuffer_src;
			*framebuffer_src = *(framebuffer_src + 1);
			*(framebuffer_src + 1) = tmp;
		}
	}


	void Initialize(SDL_Renderer* renderer)
	{
		assert(renderer != nullptr);
		Renderer::renderer = renderer;
		RecreateTexture();
		rendering_is_enabled = false;
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


	void Render()
	{
		if (!rendering_is_enabled) {
			return;
		}

		void* locked_pixels = nullptr;
		int locked_pixels_pitch = 0;
		SDL_LockTexture(texture, nullptr, &locked_pixels, &locked_pixels_pitch);

		/* RDRAM is stored in big endian. If using RGBA5551 (RGBA5553), colour channels
		   will go across byte boundaries. Thus, it is not enough to use SDL_PIXELFORMAT_ABGR1555;
		   we need to byteswap each halfword in the framebuffer before copying it into the texture. */
		if (framebuffer.pixel_format == SDL_PIXELFORMAT_RGBA5551) {
			ByteswapFramebuffer();
		}

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

		if (framebuffer.pixel_format == SDL_PIXELFORMAT_RGBA5551) {
			ByteswapFramebuffer();
		}

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);
	}


	void SetFramebufferPtr(u8* ptr)
	{
		framebuffer.src_ptr = ptr;
	}


	void SetFramebufferWidth(uint width)
	{
		assert(width > 0);
		framebuffer.width = width;
		framebuffer.height = width * 3 / 4; /* 4:3 aspect ratio */
		framebuffer.pitch = width * framebuffer.bytes_per_pixel;
		RecreateTexture();
	}


	template<PixelFormat pixel_format>
	void SetPixelFormat()
	{
		if constexpr (pixel_format == PixelFormat::Blank) {
			rendering_is_enabled = false;
		}
		else if constexpr (pixel_format == PixelFormat::RGBA5553) {
			/* Treat this as RGBA5551 */
			/* RDRAM is stored in big endian, but we do manual byteswapping before copying to texture. */
			framebuffer.pixel_format = SDL_PIXELFORMAT_RGBA5551;
			framebuffer.bytes_per_pixel = 2;
			rendering_is_enabled = true;
		}
		else if constexpr (pixel_format == PixelFormat::RGBA8888) {
			/* Here, we can use SDL_PIXELFORMAT_ABGR8888 and don't need to do manual byteswapping before copying to
			texture (we let SDL do it instead), since all colour channels are within byte boundaries, unlike with RGBA5551. */
			framebuffer.pixel_format = SDL_PIXELFORMAT_ABGR8888;
			framebuffer.bytes_per_pixel = 4;
			rendering_is_enabled = true;
		}
		else {
			static_assert(AlwaysFalse<pixel_format>);
		}
		framebuffer.pitch = framebuffer.width * framebuffer.bytes_per_pixel;
	}


	void SetRenderer(SDL_Renderer* renderer)
	{
		Renderer::renderer = renderer;
	}


	void SetWindowSize(uint width, uint height)
	{
		assert(width > 0 && height > 0);
		SDL_RenderSetLogicalSize(renderer, width, height);
	}


	template void SetPixelFormat<PixelFormat::Blank>();
	template void SetPixelFormat<PixelFormat::RGBA5553>();
	template void SetPixelFormat<PixelFormat::RGBA8888>();
}