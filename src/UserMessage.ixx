export module UserMessage;

import <cassert>;
import <string>;

import <SDL.h>;

namespace UserMessage
{
	SDL_Window* sdl_window = nullptr; /* Must be set via 'SetWindow' before any messages are shown. */

	export enum class Type { Unspecified, Success, Warning, Error, Fatal };

	export inline void SetWindow(SDL_Window* sdl_window_) { sdl_window = sdl_window_; }

	export inline void Show(const std::string& message, const Type type = Type::Unspecified)
	{
		if (sdl_window == nullptr)
		{
			assert(false);
			return;
		}

		const std::string out_msg_prefix = [&] {
			switch (type)
			{
			case Type::Success: return "Success: ";
			case Type::Warning: return "Warning: ";
			case Type::Error: return "Error: ";
			case Type::Fatal: return "Fatal: ";
			default: assert(false);
			}
		}();

		const auto sdl_msg_type = [&] {
			switch (type)
			{
			case Type::Success: return SDL_MESSAGEBOX_INFORMATION;
			case Type::Warning: return SDL_MESSAGEBOX_WARNING;
			case Type::Error: return SDL_MESSAGEBOX_ERROR;
			case Type::Fatal: return SDL_MESSAGEBOX_ERROR;
			default: assert(false);
			}
		}();

		const std::string out_msg = out_msg_prefix + message;

		SDL_ShowSimpleMessageBox(sdl_msg_type, "Message", out_msg.c_str(), sdl_window);
	}
}