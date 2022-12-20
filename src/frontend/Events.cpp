module Events;

namespace Events
{
	void Poll()
	{
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN:

				break;

			case SDL_QUIT:
				SDL_Quit();
				exit(0);
				break;

			default:
				break;
			}
		}
	}
}