export module Input;

import <SDL.h>;

namespace Input
{
	export void Poll();

	SDL_Event event;
}