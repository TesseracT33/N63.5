export module Events;

import <SDL.h>;

namespace Events
{
	export void Poll();

	SDL_Event event;
}