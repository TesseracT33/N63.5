export module FakeRDP;

import Events;
import Gui;
import RDPImplementation;
import VI;
import Util;
import RDRAM;

import "SDL.h";

import <bit>;
import <iostream>;

export class FakeRDP : public RDPImplementation {
	void EnqueueCommand(int cmd_len, u32* cmd_ptr) {};
	void OnFullSync() {};
	void TearDown() {};

	struct Framebuffer {
		u8* src_ptr{};
		uint width = 320, height = 240, pitch = 320 * 4;
		uint bytes_per_pixel = 4;
		uint size = width * height * bytes_per_pixel;
		uint pixel_format = SDL_PIXELFORMAT_ABGR8888;
	} framebuffer{};

	SDL_Renderer* renderer{};
	SDL_Texture* texture{};
	SDL_Window* sdl_window{};

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


	bool Initialize() {
		sdl_window = Gui::GetSdlWindow();
		if (!sdl_window) {
			std::cerr << __FUNCTION__ << ": retrieved SDL_Window is nullptr\n";
			return false;
		}

		renderer = SDL_CreateRenderer(sdl_window, 0, SDL_RENDERER_ACCELERATED);
		if (!renderer) {
			std::cerr << __FUNCTION__ << ": could not create SDL renderer: " << SDL_GetError() << '\n';
			return false;
		}
		framebuffer.src_ptr = RDRAM::GetPointerToMemory();
		framebuffer.pixel_format = SDL_PIXELFORMAT_RGBA5551;
		framebuffer.bytes_per_pixel = 2;
		framebuffer.pitch = framebuffer.width * framebuffer.bytes_per_pixel;
		SDL_RenderSetLogicalSize(renderer, 320, 240);

		RecreateTexture();
		return true;
	}

	void SetOrigin(u32 origin) {
		framebuffer.src_ptr = RDRAM::GetPointerToMemory(origin);
		RecreateTexture();
	}

	void SetWidth(u32 width) {
		if (width == 0) return;
		framebuffer.width = width;
		framebuffer.height = width * 3 / 4; /* 4:3 aspect ratio */
		framebuffer.pitch = width * framebuffer.bytes_per_pixel;
		RecreateTexture();
	}

	void UpdateScreen() {
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
		Events::Poll();
	}

	void SetControl(u32 control) {
		switch (control & 3) {
		case 0b00: /* blank (no data and no sync, TV screens will either show static or nothing) */
		case 0b01: /* reserved */
			break;

		case 0b10: /* 5/5/5/3 */
			framebuffer.pixel_format = SDL_PIXELFORMAT_RGBA5551;
			framebuffer.bytes_per_pixel = 2;
			break;

		case 0b11: /* 8/8/8/8 */
			framebuffer.pixel_format = SDL_PIXELFORMAT_ABGR8888;
			framebuffer.bytes_per_pixel = 4;
			break;
		}
		framebuffer.pitch = framebuffer.width * framebuffer.bytes_per_pixel;
		RecreateTexture();
	}
};