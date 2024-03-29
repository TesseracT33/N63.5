module;

#include "imgui_impl_vulkan.h"
#include "SDL.h"
#include "volk.h"

export module Gui;

import Util;

import <algorithm>;
import <array>;
import <cassert>;
import <chrono>;
import <filesystem>;
import <format>;
import <iostream>;
import <optional>;
import <string>;
import <string_view>;
import <unordered_map>;
import <utility>;
import <vector>;

namespace Gui
{
	export {
		void Frame(VkCommandBuffer vk_command_buffer); // To be called by RDP implementation while game is running
		SDL_Window* GetSdlWindow();
		void GetWindowSize(int* w, int* h);
		bool Init();
		void OnCtrlKeyPress(SDL_Keycode keycode);
		void PollEvents();
		void Run(bool boot_game_immediately = false);
		void TearDown();
	}

	namespace fs = std::filesystem;

	void Draw();
	void DrawGameSelectionWindow();
	void DrawInputBindingsWindow();
	void DrawMenu();
	void DrawRdpConfWindow();
	bool EnterFullscreen();
	bool ExitFullscreen();
	std::optional<fs::path> FileDialog();
	std::optional<fs::path> FolderDialog();
	bool InitGraphics();
	bool InitImgui();
	bool InitSdl();
	bool NeedsDraw();
	void OnGameSelected(size_t list_index);
	void OnInputBindingsWindowResetAll();
	void OnInputBindingsWindowSave();
	void OnInputBindingsWindowUseControllerDefaults();
	void OnInputBindingsWindowUseKeyboardDefaults();
	void OnMenuConfigureBindings();
	void OnMenuEnableAudio();
	void OnMenuFullscreen();
	void OnMenuLoadState();
	void OnMenuOpen();
	void OnMenuOpenBios();
	void OnMenuOpenRecent();
	void OnMenuPause();
	void OnMenuQuit();
	void OnMenuReset();
	void OnMenuSaveState();
	void OnMenuShowGameList();
	void OnMenuStop();
	void OnMenuWindowScale();
	void OnRdpWindowParallelRdpSelected();
	void OnSdlQuit();
	void OnSetGameDirectory();
	void OnWindowResizeEvent(SDL_Event const& event);
	bool ReadConfigFile();
	void RefreshGameList();
	void StartGame();
	void StopGame();
	void UpdateWindowTitle();
	void UseDefaultConfig();

	bool filter_game_list_to_n64_files;
	bool game_is_running;
	bool menu_enable_audio;
	bool menu_fullscreen;
	bool menu_pause_emulation;
	bool quit;
	bool show_game_selection_window;
	bool show_input_bindings_window;
	bool show_menu;
	bool show_rdp_conf_window;
	bool start_game;

	int frame_counter;
	int window_height, window_width;

	float fps;

	std::string current_game_title;
	fs::path game_list_directory;

	struct GameListEntry {
		fs::path path;
		std::string name; // store file name with char value type so that imgui can display it
	};

	std::vector<GameListEntry> game_list;

	SDL_Window* sdl_window;
}