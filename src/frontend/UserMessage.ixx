module;

#include "SDL.h"

export module UserMessage;

import Log;

import <format>;
import <iostream>;
import <source_location>;
import <string>;

namespace UserMessage
{
	export {
		void Error(auto const& message);
		void Fatal(auto const& message, std::source_location loc = std::source_location::current());
		void Info(auto const& message);
		bool Init(SDL_Window* sdl_window);
		void Warning(auto const& message);
	}

	SDL_Window* sdl_window; /* Must be set via 'Init' before any messages are shown. */

	bool Init(SDL_Window* sdl_window)
	{
		if (sdl_window) {
			UserMessage::sdl_window = sdl_window;
			return true;
		}
		else {
			Log::Error("nullptr given as argument to UserMessage::SetWindow");
			return false;
		}
	}

	void Error(auto const& message)
	{
		Log::Error(message);
		if (sdl_window) {
			std::string shown_message = std::string("Error: ") + message;
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", shown_message.c_str(), sdl_window);
		}
	}

	void Fatal(auto const& message, std::source_location loc)
	{
		Log::Fatal(message, loc);
		if (sdl_window) {
			std::string shown_message = std::format("Fatal error at {}({}:{}), function {}: {}",
				loc.file_name(), loc.line(), loc.column(), loc.function_name(), message);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal", shown_message.c_str(), sdl_window);
		}
	}

	void Info(auto const& message)
	{
		Log::Info(message);
		if (sdl_window) {
			std::string shown_message = std::string("Info: ") + message;
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Information", shown_message.c_str(), sdl_window);
		}
	}

	void Warning(auto const& message)
	{
		Log::Warning(message);
		if (sdl_window) {
			std::string shown_message = std::string("Warning: ") + message;
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning", shown_message.c_str(), sdl_window);
		}
	}
}