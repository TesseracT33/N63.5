module;

#include "imgui_impl_vulkan.h"
#include "vulkan/vulkan.h"

export module Gui;

import Util;

import "SDL.h";

import <chrono>;
import <format>;
import <iostream>;
import <thread>;
import <utility>;

export enum class RenderBackend {
	Sdl, Vulkan
};

// TODO: To be selectable
RenderBackend render_backend = RenderBackend::Vulkan;

namespace Gui
{
	export
	{
		SDL_Window* GetSdlWindow();
		bool Init();
		void OnCtrlKeyPress(SDL_Keycode keycode);
		void OnSdlQuit();
		void Run(bool boot_game_immediately = false);
		void TearDown();
	}

	float GetImGuiMenuBarHeight();
	bool InitGraphics();
	bool InitImgui();
	bool InitSdl();
	void OnMenuConfigureBindings();
	void OnMenuEnableAudio();
	void OnMenuFullscreen();
	void OnMenuLoadState();
	void OnMenuLockFramerate();
	void OnMenuOpen();
	void OnMenuOpenBios();
	void OnMenuOpenRecent();
	void OnMenuPause();
	void OnMenuQuit();
	void OnMenuReset();
	void OnMenuSaveState();
	void OnMenuStop();
	void OnMenuWindowScale();
	void RenderMenu();
	void RenderInputBindingsWindow();
	void ScheduleEmuThread(void(*function)());
	void StartGame();
	void StopGame();

	bool input_window_button_pressed;
	bool menu_enable_audio;
	bool menu_fullscreen;
	bool menu_lock_framerate;
	bool menu_pause_emulation;
	bool quit;
	bool show_input_bindings_window;
	bool show_menu;

	VkAllocationCallbacks*   vk_allocator;
	VkInstance               vk_instance;
	VkPhysicalDevice         vk_physical_device;
	VkDevice                 vk_device;
	u32                      vk_queue_family;
	VkQueue                  vk_queue;
	VkPipelineCache          vk_pipeline_cache;
	VkDescriptorPool         vk_descriptor_pool;
	ImGui_ImplVulkanH_Window vk_main_window_data;
	u32                      vk_min_image_count;
	bool                     vk_swap_chain_rebuild;
	VkRenderPass             vk_render_pass;

	std::jthread emu_thread;

	SDL_Window* sdl_window;
}