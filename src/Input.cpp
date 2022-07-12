module Input;

namespace Input
{
	void Poll()
	{
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
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