module Renderer;

import HostSystem;

namespace Renderer
{
	void Initialize(SDL_Renderer* renderer)
	{
		Renderer::renderer = renderer;
	}


	void Render()
	{
		int width = 320;
		int height = 240;
		int depth = 8 * 4;
		int pitch = 240 * 4;
		SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(framebuffer_ptr, width, height, depth, pitch, maskR, maskG, maskB, maskA);

		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

		SDL_RenderCopy(renderer, texture, nullptr, nullptr);

		SDL_RenderPresent(renderer);

		SDL_FreeSurface(surface);
		SDL_DestroyTexture(texture);
	}


	void SetColourFormat(unsigned num_red_bits, unsigned num_green_bits, unsigned num_blue_bits, unsigned num_alpha_bits)
	{
		assert(num_red_bits <= 8 && num_green_bits <= 8 && num_blue_bits <= 8 && num_alpha_bits <= 8);

		if (HostSystem::endianness == std::endian::little)
		{
			maskR = num_red_bits   > 0 ? ((1 << num_red_bits  ) - 1) << 24 : 0;
			maskG = num_green_bits > 0 ? ((1 << num_green_bits) - 1) << 16 : 0;
			maskB = num_blue_bits  > 0 ? ((1 << num_blue_bits ) - 1) <<  8 : 0;
			maskA = num_alpha_bits > 0 ? ((1 << num_alpha_bits) - 1)       : 0;
		}
		else
		{
			maskR = num_red_bits   > 0 ? ((1 << num_red_bits  ) - 1)       : 0;
			maskG = num_green_bits > 0 ? ((1 << num_green_bits) - 1) <<  8 : 0;
			maskB = num_blue_bits  > 0 ? ((1 << num_blue_bits ) - 1) << 16 : 0;
			maskA = num_alpha_bits > 0 ? ((1 << num_alpha_bits) - 1) << 24 : 0;
		}
	}


	void SetFramebufferPtr(u8* ptr)
	{
		framebuffer_ptr = ptr;
	}


	void SetRenderer(SDL_Renderer* renderer)
	{
		Renderer::renderer = renderer;
	}


	void SetWindowSize(int width, int height)
	{
		assert(width > 0 && height > 0);
		SDL_RenderSetLogicalSize(renderer, width, height);
	}
}