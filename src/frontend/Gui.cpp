module;

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include "imgui_impl_vulkan.h"

module Gui;

import Audio;
import BuildOptions;
import Events;
import N64;
import UserMessage;
import Vulkan;

namespace Gui
{
	float GetImGuiMenuBarHeight()
	{
		// TODO
		return 19;
	}


	SDL_Window* GetSdlWindow()
	{
		if (!sdl_window) {
			InitSdl();
		}
		return sdl_window;
	}


	bool Init()
	{
		if (!InitSdl()) {
			std::cerr << "[fatal] Failed to init SDL!\n";
			return false;
		}
		if (!InitGraphics()) {
			std::cerr << "[fatal] Failed to init graphics system!\n";
			return false;
		}
		if (!InitImgui()) {
			std::cerr << "[fatal] Failed to init ImGui!\n";
			return false;
		}
		if (!Audio::Init()) {
			std::cerr << "[error] Failed to init audio system!\n";
		}

		auto menubar_height = GetImGuiMenuBarHeight();

		input_window_button_pressed = false;
		menu_enable_audio = true;
		menu_fullscreen = false;
		menu_lock_framerate = true;
		menu_pause_emulation = false;
		quit = false;
		show_input_bindings_window = false;
		show_menu = true;

		return true;
	}


	bool InitGraphics()
	{
		switch (render_backend) {
		case RenderBackend::Sdl:
			return true;
		case RenderBackend::Vulkan:
			return Vulkan::Init(sdl_window);
		default:
			return false;
		}
	}


	bool InitImgui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImGui::StyleColorsDark();

		if (render_backend == RenderBackend::Sdl) {
			return true; // TODO
		}
		else if (render_backend == RenderBackend::Vulkan) {
			ImGui_ImplSDL2_InitForVulkan(sdl_window);

			ImGui_ImplVulkan_InitInfo init_info = {
				.Instance = vk_instance,
				.PhysicalDevice = vk_physical_device,
				.Device = vk_device,
				.QueueFamily = vk_queue_family,
				.Queue = vk_queue,
				.PipelineCache = vk_pipeline_cache,
				.DescriptorPool = vk_descriptor_pool,
				.Subpass = 0,
				.MinImageCount = vk_min_image_count,
				.ImageCount = 2,
				.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
				.Allocator = vk_allocator,
				.CheckVkResultFn = Vulkan::CheckVkResult
			};

			if (!ImGui_ImplVulkan_Init(&init_info, vk_render_pass)) {
				UserMessage::Fatal("Failed call to ImGui_ImplVulkan_Init.");
				return false;
			}

			io.Fonts->AddFontDefault();
			// Upload Fonts
			{
				ImGui_ImplVulkan_CreateFontsTexture(Vulkan::GetVkCommandBuffer());
				Vulkan::SubmitRequestedVkCommandBuffer();
			}

			return true;
		}
		else {
			return false;
		}
	}


	bool InitSdl()
	{
		SDL_SetMainReady();
		if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
			std::cerr << std::format("Failed to init SDL: {}\n", SDL_GetError());
			return false;
		}
		auto window_flags = SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
		if (render_backend == RenderBackend::Vulkan) {
			window_flags |= SDL_WINDOW_VULKAN;
		}
		sdl_window = SDL_CreateWindow("N63.5", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 720, window_flags);
		if (!sdl_window) {
			std::cerr << std::format("Failed to create SDL window: {}\n", SDL_GetError());
			return false;
		}
		if (!UserMessage::Initialize(sdl_window)) {
			std::cerr << "Failed to initialize user message system\n";
		}
		return true;
	}


	void OnCtrlKeyPress(SDL_Keycode keycode)
	{
		switch (keycode) {
		case SDLK_a:
			menu_enable_audio = !menu_enable_audio;
			OnMenuEnableAudio();
			break;

		case SDLK_f:
			menu_lock_framerate = !menu_lock_framerate;
			OnMenuLockFramerate();
			break;

		case SDLK_l:
			OnMenuLoadState();
			break;

		case SDLK_m:
			show_menu = !show_menu;
			//Video::SetGameRenderAreaOffsetY(show_gui ? 19 : 0); // TODO: make non-hacky
			break;

		case SDLK_o:
			OnMenuOpen();
			break;

		case SDLK_p:
			menu_pause_emulation = !menu_pause_emulation;
			OnMenuPause();
			break;

		case SDLK_q:
			OnMenuQuit();
			break;

		case SDLK_r:
			OnMenuReset();
			break;

		case SDLK_RETURN:
			menu_fullscreen = !menu_fullscreen;
			OnMenuFullscreen();
			break;

		case SDLK_s:
			OnMenuSaveState();
			break;

		case SDLK_x:
			OnMenuStop();
			break;

		default:
			break;
		}
	}


	void OnMenuConfigureBindings()
	{
		show_input_bindings_window = !show_input_bindings_window;
	}


	void OnMenuEnableAudio()
	{
		menu_enable_audio ? Audio::Enable() : Audio::Disable();
	}


	void OnMenuFullscreen()
	{
		//menu_fullscreen ? Video::EnableFullscreen() : Video::DisableFullscreen();
	}


	void OnMenuLoadState()
	{
		N64::LoadState();
	}


	void OnMenuLockFramerate()
	{
		//menu_lock_framerate ? N64::LockFramerate() : N64::UnlockFramerate();
	}


	void OnMenuOpen()
	{
		//if (!N64::LoadRom(rom_path)) {
		//	UserMessage::Warning(std::format("Could not load rom at path \"{}\"", rom_path));
		//}
	}


	void OnMenuOpenBios()
	{
		//if (!N64::LoadBios(bios_path)) {
		//	UserMessage::Warning(std::format("Could not load bios at path \"{}\"", bios_path));
		//}
	}


	void OnMenuOpenRecent()
	{
		// TODO
	}


	void OnMenuPause()
	{
		menu_pause_emulation ? N64::Pause() : ScheduleEmuThread(N64::Resume);
	}


	void OnMenuQuit()
	{
		N64::Stop();
		if (emu_thread.joinable()) {
			emu_thread.join();
		}
		quit = true;
	}


	void OnMenuReset()
	{
		ScheduleEmuThread(N64::Reset);
	}


	void OnMenuSaveState()
	{
		N64::SaveState();
	}


	void OnMenuStop()
	{
		N64::Stop();
	}


	void OnMenuWindowScale()
	{
		// TODO
	}


	void OnSdlQuit()
	{
		quit = true;
	}


	void RenderMenu()
	{
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Open", "Ctrl+O")) {
					OnMenuOpen();
				}
				if (ImGui::MenuItem("Open recent")) {
					OnMenuOpenRecent();
				}
				if (ImGui::MenuItem("Open BIOS")) {
					OnMenuOpenBios();
				}
				if (ImGui::MenuItem("Load state", "Ctrl+L")) {
					OnMenuLoadState();
				}
				if (ImGui::MenuItem("Save state", "Ctrl+S")) {
					OnMenuSaveState();
				}
				if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
					OnMenuQuit();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Emulation")) {
				if (ImGui::MenuItem("Pause", "Ctrl+P", &menu_pause_emulation, true)) {
					OnMenuPause();
				}
				if (ImGui::MenuItem("Reset", "Ctrl+R")) {
					OnMenuReset();
				}
				if (ImGui::MenuItem("Stop", "Ctrl+X")) {
					OnMenuStop();
				}
				if (ImGui::MenuItem("Lock framerate", "Ctrl+F", &menu_lock_framerate, true)) {
					OnMenuLockFramerate();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Audio")) {
				if (ImGui::MenuItem("Enable", "Ctrl+A", &menu_enable_audio, true)) {
					OnMenuEnableAudio();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Video")) {
				if (ImGui::MenuItem("Set window scale")) {
					OnMenuWindowScale();
				}
				if (ImGui::MenuItem("Fullscreen", "Ctrl+Enter", &menu_fullscreen, true)) {
					OnMenuFullscreen();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Input")) {
				if (ImGui::MenuItem("Configure bindings")) {
					OnMenuConfigureBindings();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Debug")) {
				// TODO
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		if (show_input_bindings_window) {
			RenderInputBindingsWindow();
		}
	}


	void RenderInputBindingsWindow()
	{
		// TODO
	}


	void Run(bool boot_game_immediately)
	{
		if (boot_game_immediately) {
			StartGame();
		}
		while (!quit) {
			Events::Poll();

			if (vk_swap_chain_rebuild) {
				int w, h;
				SDL_GetWindowSize(sdl_window, &w, &h);
				if (w > 0 && h > 0) {
					ImGui_ImplVulkan_SetMinImageCount(vk_min_image_count);
					ImGui_ImplVulkanH_CreateOrResizeWindow(vk_instance, vk_physical_device, vk_device, &vk_main_window_data, vk_queue_family, vk_allocator, w, h, vk_min_image_count);
					vk_main_window_data.FrameIndex = 0;
					vk_swap_chain_rebuild = false;
				}
			}

			// Start the Dear ImGui frame
			if (render_backend == RenderBackend::Vulkan) {
				ImGui_ImplVulkan_NewFrame();
			}
			else if (render_backend == RenderBackend::Sdl) {
				// TODO
			}
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();
			N64::UpdateScreen();
			if (show_menu) {
				RenderMenu();
			}
			ImGui::Render();
			ImDrawData* draw_data = ImGui::GetDrawData();
			bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
			if (!is_minimized) {
				//wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
				//wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
				//wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
				//wd->ClearValue.color.float32[3] = clear_color.w;
				//FrameRender(wd, draw_data);
				//FramePresent(wd);
			}
			//SDL_GL_SwapWindow(sdl_window);

			/* SDL will automatically block so that the number of frames rendered per second is
			   equal to the display's refresh rate. */
		}
	}


	void ScheduleEmuThread(void(*function)())
	{
		N64::Stop();
		if (emu_thread.joinable()) {
			emu_thread.join();
		}
		emu_thread = std::jthread{ function };
	}


	void StartGame()
	{
		ScheduleEmuThread(N64::Run);
	}


	void StopGame()
	{
		N64::Stop();
	}


	void TearDown()
	{
		N64::Stop();
		/*vk_result = vkDeviceWaitIdle(vk_device);
		CheckVkResult(vk_result);
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
		ImGui_ImplVulkanH_DestroyWindow(vk_instance, vk_device, &vk_main_window_data, vk_allocator);
		vkDestroyDescriptorPool(vk_device, vk_descriptor_pool, vk_allocator);
		vkDestroyDevice(vk_device, vk_allocator);
		vkDestroyInstance(vk_instance, vk_allocator);
		SDL_DestroyWindow(sdl_window);
		SDL_Quit();*/
	}
}