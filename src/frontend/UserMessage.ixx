export module UserMessage;

import "SDL.h";

import <iostream>;
import <string>;

namespace UserMessage
{
	SDL_Window* sdl_window; /* Must be set via 'Init' before any messages are shown. */

	export bool Init(SDL_Window* sdl_window)
	{
		if (sdl_window) {
			UserMessage::sdl_window = sdl_window;
			return true;
		}
		else {
			std::cerr << "nullptr given as argument to UserMessage::SetWindow\n";
			return false;
		}
	}

	export void Error(auto const& message)
	{
		if (sdl_window) {
			std::string shown_message = std::string("Error: ") + message;
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", shown_message.c_str(), sdl_window);
		}
	}

	export void Fatal(auto const& message)
	{
		if (sdl_window) {
			std::string shown_message = std::string("Fatal: ") + message;
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal", shown_message.c_str(), sdl_window);
		}
	}

	export void Info(auto const& message)
	{
		if (sdl_window) {
			std::string shown_message = std::string("Info: ") + message;
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Information", shown_message.c_str(), sdl_window);
		}
	}

	export void Warning(auto const& message)
	{
		if (sdl_window) {
			std::string shown_message = std::string("Warning: ") + message;
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning", shown_message.c_str(), sdl_window);
		}
	}
}