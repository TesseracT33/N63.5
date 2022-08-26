export module UserMessage;

import <SDL.h>;

import <cassert>;
import <format>;
import <iostream>;
import <string>;

namespace UserMessage
{
	SDL_Window* sdl_window; /* Must be set via 'SetWindow' before any messages are shown. */

	export
	{
		enum class Type {
			Unspecified, Success, Warning, Error, Fatal
		};

		void SetWindow(SDL_Window* sdl_window)
		{
			if (!sdl_window) {
				std::cerr << "nullptr given as argument to UserMessage::SetWindow\n";
				assert(false);
			}
			UserMessage::sdl_window = sdl_window;
		}

		void Show(const std::string& message, Type type = Type::Unspecified)
		{
			std::string out_msg_prefix = [&] {
				switch (type) {
				case Type::Success: return "Success: ";
				case Type::Warning: return "Warning: ";
				case Type::Error: return "Error: ";
				case Type::Fatal: return "Fatal: ";
				default: return "";
				}
			}();

			auto sdl_msg_type = [&] {
				switch (type) {
				case Type::Success: return SDL_MESSAGEBOX_INFORMATION;
				case Type::Warning: return SDL_MESSAGEBOX_WARNING;
				case Type::Error: return SDL_MESSAGEBOX_ERROR;
				case Type::Fatal: return SDL_MESSAGEBOX_ERROR;
				default: return SDL_MESSAGEBOX_INFORMATION;
				}
			}();

			std::string out_msg = out_msg_prefix + message;

			SDL_ShowSimpleMessageBox(sdl_msg_type, "Message", out_msg.c_str(), sdl_window);
		}
	}
}