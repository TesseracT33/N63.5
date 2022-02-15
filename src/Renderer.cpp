module Renderer;

namespace Renderer
{
	void Render()
	{
		int width = 320;
		int height = 240;
		int depth = 8 * 4;
		int pitch = 240 * 4;
		u32 Rmask = 0x0000'00FF, Gmask = 0x0000'FF00, Bmask = 0x00FF'0000, Amask = 0xFF00'0000;
		SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(framebuffer_ptr, width, height, depth, pitch, Rmask, Gmask, Bmask, Amask);

		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

		SDL_RenderCopy(renderer, texture, nullptr, nullptr);

		SDL_RenderPresent(renderer);

		SDL_FreeSurface(surface);
		SDL_DestroyTexture(texture);
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