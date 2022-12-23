module;

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_vulkan.h"

module Gui;

import Audio;
import Events;
import N64;
import UserMessage;

namespace Gui
{
	void CheckVkResult(VkResult vk_result)
	{
		if (vk_result != 0) {
			std::cerr << "[vulkan]: Error: VkResult = " << int(vk_result) << '\n';
			if (vk_result < 0) {
				exit(1);
			}
		}
	}


	float GetImGuiMenuBarHeight()
	{
		// TODO
		return 19;
	}


	bool Initialize()
	{
		if (!InitSdl()) {
			return false;
		}
		if (!InitVulkan()) {
			return false;
		}
		if (!InitImgui()) {
			return false;
		}

		auto menubar_height = GetImGuiMenuBarHeight();

		input_window_button_pressed = false;
		menu_enable_audio = true;
		menu_fullscreen = false;
		menu_lock_framerate = true;
		menu_pause_emulation = false;
		quit = false;
		show_input_bindings_window = false;
		show_menubar = true;

		return true;
	}


	bool InitSdl()
	{
		SDL_SetMainReady();
		if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
			std::cerr << std::format("Failed to init SDL: {}\n", SDL_GetError());
			return false;
		}
		sdl_window = SDL_CreateWindow("N63.5", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			960, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_RESIZABLE);
		if (!sdl_window) {
			std::cerr << std::format("Failed to create SDL window: {}\n", SDL_GetError());
			return false;
		}
		if (!UserMessage::Initialize(sdl_window)) {
			std::cerr << "Failed to initialize user message system\n";
		}
		return true;
	}


	bool InitImgui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImGui::StyleColorsDark();

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
			.CheckVkResultFn = CheckVkResult
		};

		if (!ImGui_ImplVulkan_Init(&init_info, vk_render_pass)) {
			UserMessage::ShowFatal("Failed call to ImGui_ImplVulkan_Init.");
			return false;
		}

		io.Fonts->AddFontDefault();
		// Upload Fonts
		{
			VkCommandBuffer command_buffer = get_vk_command_buffer();
			ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
			submit_requested_vk_command_buffer();
		}
	}


	bool InitVulkan()
	{
		// TODO: init parallel-rdp

		vk_instance = get_vk_instance();
		vk_physical_device = get_vk_physical_device();
		vk_device = get_vk_device();
		vk_queue_family = get_vk_graphics_queue_family();
		vk_queue = get_graphics_queue();
		vk_pipeline_cache = nullptr;
		vk_descriptor_pool = nullptr;
		vk_allocator = nullptr;
		vk_min_image_count = 2;

		VkResult vk_result;

		{ // Create Descriptor Pool
			VkDescriptorPoolSize pool_sizes[] = {
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};
			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
			pool_info.poolSizeCount = (u32)IM_ARRAYSIZE(pool_sizes);
			pool_info.pPoolSizes = pool_sizes;
			vk_result = vkCreateDescriptorPool(vk_device, &pool_info, vk_allocator, &vk_descriptor_pool);
			CheckVkResult(vk_result);
		}

		// Create the Render Pass
		{
			VkAttachmentDescription attachment = {};
			attachment.format = get_vk_format();
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			VkAttachmentReference color_attachment = {};
			color_attachment.attachment = 0;
			color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &color_attachment;
			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			VkRenderPassCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			info.attachmentCount = 1;
			info.pAttachments = &attachment;
			info.subpassCount = 1;
			info.pSubpasses = &subpass;
			info.dependencyCount = 1;
			info.pDependencies = &dependency;
			vk_result = vkCreateRenderPass(vk_device, &info, vk_allocator, &vk_render_pass);
			CheckVkResult(vk_result);
		}
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
			show_menubar = !show_menubar;
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
		//	UserMessage::ShowWarning(std::format("Could not load rom at path \"{}\"", rom_path));
		//}
	}


	void OnMenuOpenBios()
	{
		//if (!N64::LoadBios(bios_path)) {
		//	UserMessage::ShowWarning(std::format("Could not load bios at path \"{}\"", bios_path));
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


	void Render()
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
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();
			N64::UpdateScreen();
			if (show_menubar) {
				Render();
			}
			ImGui::Render();
			ImDrawData* draw_data = ImGui::GetDrawData();
			bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
			if (!is_minimized) {
				wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
				wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
				wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
				wd->ClearValue.color.float32[3] = clear_color.w;
				FrameRender(wd, draw_data);
				FramePresent(wd);
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
		VkResult vk_result;
		N64::Stop();
		vk_result = vkDeviceWaitIdle(vk_device);
		CheckVkResult(vk_result);
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
		ImGui_ImplVulkanH_DestroyWindow(vk_instance, vk_device, &vk_main_window_data, vk_allocator);
		vkDestroyDescriptorPool(vk_device, vk_descriptor_pool, vk_allocator);
		vkDestroyDevice(vk_device, vk_allocator);
		vkDestroyInstance(vk_instance, vk_allocator);
		SDL_DestroyWindow(sdl_window);
		SDL_Quit();
	}
}