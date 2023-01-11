module;

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_vulkan.h"
#include "nfd.h"
#include "volk.h"

module Gui;

import Audio;
import BuildOptions;
import Input;
import N64;
import RDP;
import UserMessage;
import Vulkan;

namespace fs = std::filesystem;
namespace rng = std::ranges;

void Gui::Draw()
{
	if (show_menu) {
		DrawMenu();
	}
	if (show_game_selection_window) {
		DrawGameSelectionWindow();
	}
	if (show_input_bindings_window) {
		DrawInputBindingsWindow();
	}
	if (show_rdp_conf_window) {
		DrawRdpConfWindow();
	}
}

void Gui::DrawGameSelectionWindow()
{
	if (ImGui::Begin("Game selection", &show_game_selection_window)) {
		if (ImGui::Button("Set game directory")) {
			OnSetGameDirectory();
		}
		ImGui::SameLine();
		if (ImGui::Checkbox("Filter to common n64 file types", &filter_game_list_to_n64_files)) {
			RefreshGameList();
		}
		if (game_list_directory.empty()) {
			ImGui::Text("No directory set!");
		}
		else {
			// TODO: see if conversion to char* which ImGui requires can be done in a better way.
			// On Windows: directories containing non-ASCII characters display incorrectly
			ImGui::Text(game_list_directory.string().c_str());
		}
		if (ImGui::BeginListBox("Game selection")) {
			static int item_current_idx = 0; // Here we store our selection data as an index.
			for (size_t n = 0; n < game_list.size(); ++n) {
				bool is_selected = item_current_idx == n;
				if (ImGui::Selectable(game_list[n].name.c_str(), is_selected, ImGuiSelectableFlags_AllowDoubleClick)) {
					item_current_idx = n;
					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
						OnGameSelected(size_t(n));
					}
				}
				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndListBox();
		}
		ImGui::End();
	}
}

void Gui::DrawInputBindingsWindow()
{
	static constexpr std::string_view unbound_label = "Unbound";
	static constexpr std::string_view waiting_for_input_label = "...";

	struct Button {
		std::string_view const text_label;
		std::string_view button_label = unbound_label; // can be non-owning thanks to SDL's SDL_GameControllerGetStringForButton etc
		std::string_view prev_button_label = button_label;
		bool waiting_for_input = false;
	};

	static constinit std::array control_buttons = {
		Button{"A"},
		Button{"B"},
		Button{"START"},
		Button{"Z"},
		Button{"Shoulder L"},
		Button{"Shoulder R"},
		Button{"D-pad up"},
		Button{"D-pad down"},
		Button{"D-pad left"},
		Button{"D-pad right"},
		Button{"C up"},
		Button{"C down"},
		Button{"C left"},
		Button{"C right"},
		Button{"Joy X"},
		Button{"Joy Y"}
	};

	static constinit std::array hotkey_buttons = {
		Button{"Load state"},
		Button{"Save state"},
		Button{"Toggle fullscreen"}
	};

	auto OnButtonPressed = [](Button& button) {
		if (button.waiting_for_input) {
			button.button_label = button.prev_button_label;
		}
		else {
			button.prev_button_label = button.button_label;
			button.button_label = waiting_for_input_label;
		}
		button.waiting_for_input = !button.waiting_for_input;
	};

	if (ImGui::Begin("Input configuration", &show_input_bindings_window)) {
		if (ImGui::Button("Reset all")) {
			OnInputBindingsWindowResetAll();
		}
		ImGui::SameLine();
		if (ImGui::Button("Save")) {
			OnInputBindingsWindowSave();
		}
		if (ImGui::Button("Use controller defaults")) {
			OnInputBindingsWindowUseControllerDefaults();
		}
		ImGui::SameLine();
		if (ImGui::Button("Use keyboard defaults")) {
			OnInputBindingsWindowUseKeyboardDefaults();
		}

		ImGui::Separator();

		static constexpr size_t num_horizontal_elements = std::max(control_buttons.size(), hotkey_buttons.size());
		for (size_t i = 0; i < num_horizontal_elements; ++i) {
			if (i < control_buttons.size()) {
				Button& button = control_buttons[i];
				ImGui::Text(button.text_label.data());
				ImGui::SameLine(100);
				if (ImGui::Button(button.button_label.data())) {
					OnButtonPressed(button);
				}
			}
			if (i < hotkey_buttons.size()) {
				Button& button = control_buttons[i];
				ImGui::SameLine();
				ImGui::Text(button.text_label.data());
				ImGui::SameLine(250);
				if (ImGui::Button(button.button_label.data())) {
					OnButtonPressed(button);
				}
			}
		}

		ImGui::End();
	}
}

void Gui::DrawMenu()
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
			if (ImGui::MenuItem(show_game_selection_window ? "Hide game list" : "Show game list")) {
				OnMenuShowGameList();
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
			ImGui::MenuItem("RDP settings", nullptr, &show_rdp_conf_window, true);
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
}

void Gui::DrawRdpConfWindow()
{
	if (ImGui::Begin("RDP configuration", &show_rdp_conf_window)) {
		ImGui::Text("Implementation");
		{
			static int e = 0;
			if (ImGui::RadioButton("Parallel RDP (Vulkan)", &e, 0)) {
				OnRdpWindowParallelRdpSelected();
			}
		}
		ImGui::Separator();
		// TODO: parallel-rdp-specific settings
	}
	ImGui::End();
}

bool Gui::EnterFullscreen()
{
	if (SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_FULLSCREEN)) {
		UserMessage::Error(std::format("Failed to enter fullscreen: {}", SDL_GetError()));
		return false;
	}
	return true;
}

bool Gui::ExitFullscreen()
{
	SDL_SetWindowFullscreen(sdl_window, 0);
	return true;
}

std::optional<fs::path> Gui::FileDialog()
{
	nfdnchar_t* path{};
	nfdresult_t result = NFD_OpenDialogN(&path, nullptr, 0, fs::current_path().c_str());
	if (result == NFD_OKAY) {
		fs::path fs_path{ path };
		NFD_FreePathN(path);
		return fs_path;
	}
	else if (result == NFD_ERROR) {
		UserMessage::Error("nativefiledialog returned NFD_ERROR for NFD_OpenDialogN");
	}
	return {};
}

std::optional<fs::path> Gui::FolderDialog()
{
	nfdnchar_t* path{};
	nfdresult_t result = NFD_PickFolderN(&path, fs::current_path().c_str());
	if (result == NFD_OKAY) {
		fs::path fs_path{ path };
		NFD_FreePathN(path);
		return fs_path;
	}
	else if (result == NFD_ERROR) {
		UserMessage::Error("nativefiledialog returned NFD_ERROR for NFD_PickFolderN");
	}
	return {};
}

void Gui::Frame(VkCommandBuffer vk_command_buffer)
{
	PollEvents();
	if (NeedsDraw()) {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		Draw();
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vk_command_buffer);
	}
	if (++frame_counter == 60) {
		static std::chrono::time_point time = std::chrono::steady_clock::now();
		auto microsecs_to_render_60_frames = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::steady_clock::now() - time).count();
		fps = 60.0f * 1'000'000.0f / float(microsecs_to_render_60_frames);
		UpdateWindowTitle();
		frame_counter = 0;
		time = std::chrono::steady_clock::now();
	}
}

SDL_Window* Gui::GetSdlWindow()
{
	if (!sdl_window && !InitSdl()) {
		std::cerr << "[fatal] Failed to init SDL!\n";
	}
	return sdl_window;
}

void Gui::GetWindowSize(int* w, int* h)
{
	SDL_GetWindowSize(sdl_window, w, h);
}

bool Gui::Init()
{
	window_width = 640, window_height = 480;

	game_is_running = false;
	menu_enable_audio = true;
	menu_fullscreen = false;
	menu_pause_emulation = false;
	quit = false;
	show_input_bindings_window = false;
	show_menu = true;
	show_rdp_conf_window = false;
	start_game = false;

	show_game_selection_window = !game_is_running;

	if (!ReadConfigFile()) {
		std::cerr << "[error] Failed to read user configuration file! Using default settings.\n";
		UseDefaultConfig();
	}
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
	if (!Input::Init()) {
		std::cerr << "[error] Failed to init input system!\n";
	}
	if (nfdresult_t result = NFD_Init(); result != NFD_OKAY) {
		std::cerr << "[error] Failed to init nativefiledialog; NFD_Init returned " << std::to_underlying(result) << '\n';
	}
	UpdateWindowTitle();

	return true;
}

bool Gui::InitGraphics()
{
	if (!RDP::MakeParallelRdp()) {
		UserMessage::Error(std::format("Failed to initialize the RDP!"));
		return false;
	}
	return true;
}

bool Gui::InitImgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui::StyleColorsDark();

	ImGui_ImplVulkan_LoadFunctions([](const char* fn, void*) { return vkGetInstanceProcAddr(Vulkan::GetInstance(), fn); });

	if (!ImGui_ImplSDL2_InitForVulkan(sdl_window)) {
		UserMessage::Fatal("ImGui_ImplSDL2_InitForVulkan failed");
		return false;
	}

	ImGui_ImplVulkan_InitInfo init_info = {
		.Instance = Vulkan::GetInstance(),
		.PhysicalDevice = Vulkan::GetPhysicalDevice(),
		.Device = Vulkan::GetDevice(),
		.QueueFamily = Vulkan::GetGraphicsQueueFamily(),
		.Queue = Vulkan::GetQueue(),
		.PipelineCache = Vulkan::GetPipelineCache(),
		.DescriptorPool = Vulkan::GetDescriptorPool(),
		.MinImageCount = 2,
		.ImageCount = 2,
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.Allocator = Vulkan::GetAllocator(),
		.CheckVkResultFn = Vulkan::CheckVkResult
	};

	if (!ImGui_ImplVulkan_Init(&init_info, Vulkan::GetRenderPass())) {
		UserMessage::Fatal("ImGui_ImplVulkan_Init failed");
		return false;
	}

	io.Fonts->AddFontDefault();
	ImGui_ImplVulkan_CreateFontsTexture(Vulkan::GetCommandBuffer());
	Vulkan::SubmitRequestedCommandBuffer();

	return true;
}

bool Gui::InitSdl()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		std::cerr << std::format("Failed to init SDL: {}\n", SDL_GetError());
		return false;
	}
	sdl_window = SDL_CreateWindow("N63.5", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		window_width, window_height, SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN);
	if (!sdl_window) {
		std::cerr << std::format("Failed to create SDL window: {}\n", SDL_GetError());
		return false;
	}
	if (!UserMessage::Init(sdl_window)) {
		std::cerr << "Failed to initialize user message system!\n";
	}
	return true;
}

bool Gui::NeedsDraw()
{
	return show_menu || show_input_bindings_window || show_game_selection_window || show_rdp_conf_window;
}

void Gui::OnCtrlKeyPress(SDL_Keycode keycode)
{
	switch (keycode) {
	case SDLK_a:
		menu_enable_audio = !menu_enable_audio;
		OnMenuEnableAudio();
		break;

	case SDLK_l:
		OnMenuLoadState();
		break;

	case SDLK_m:
		show_menu = !show_menu;
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
	}
}

void Gui::OnGameSelected(size_t list_index)
{
	StopGame();
	if (N64::LoadGame(game_list[list_index].path)) {
		current_game_title = game_list[list_index].name;
		start_game = true;
	}
	else {
		UserMessage::Error("Failed to load game!");
	}
}

void Gui::OnInputBindingsWindowResetAll()
{
	// TODO
}

void Gui::OnInputBindingsWindowSave()
{
	if (!Input::SaveBindingsToDisk()) {
		UserMessage::Error("Failed to save input bindings!");
	}
}

void Gui::OnInputBindingsWindowUseControllerDefaults()
{
	// TODO
}

void Gui::OnInputBindingsWindowUseKeyboardDefaults()
{
	// TODO
}

void Gui::OnMenuConfigureBindings()
{
	show_input_bindings_window = !show_input_bindings_window;
}

void Gui::OnMenuEnableAudio()
{
	menu_enable_audio ? Audio::Enable() : Audio::Disable();
}

void Gui::OnMenuFullscreen()
{
	bool success = menu_fullscreen ? EnterFullscreen() : ExitFullscreen();
	if (!success) {
		menu_fullscreen = !menu_fullscreen; // revert change
	}
}

void Gui::OnMenuLoadState()
{
	N64::LoadState();
}

void Gui::OnMenuOpen()
{
	std::optional<fs::path> path = FileDialog();
	if (path.has_value()) {
		fs::path path_val = std::move(path.value());
		if (N64::LoadGame(path_val)) {
			if (game_is_running) {
				StopGame();
			}
			start_game = true;
			current_game_title = path_val.filename().string();
			game_list_directory = path_val.parent_path();
			RefreshGameList();
		}
		else {
			UserMessage::Error(std::format("Could not load rom at path \"{}\"", path_val.string()));
		}
	}
}

void Gui::OnMenuOpenBios()
{
	std::optional<fs::path> path = FileDialog();
	if (path.has_value()) {
		fs::path path_val = std::move(path.value());
		if (!N64::LoadBios(path_val)) {
			UserMessage::Error(std::format("Could not load bios at path \"{}\"", path_val.string()));
		}
	}
}

void Gui::OnMenuOpenRecent()
{
	// TODO
}

void Gui::OnMenuPause()
{
	menu_pause_emulation ? N64::Pause() : N64::Resume();
}

void Gui::OnMenuQuit()
{
	OnSdlQuit();
}

void Gui::OnMenuReset()
{
	N64::Reset();
}

void Gui::OnMenuSaveState()
{
	N64::SaveState();
}

void Gui::OnMenuShowGameList()
{
	show_game_selection_window = !show_game_selection_window;
}

void Gui::OnMenuStop()
{
	StopGame();
}

void Gui::OnMenuWindowScale()
{
	// TODO
}

void Gui::OnSetGameDirectory()
{
	std::optional<fs::path> dir = FolderDialog();
	if (dir.has_value()) {
		game_list_directory = std::move(dir.value());
		RefreshGameList();
	}
}

void Gui::OnWindowResizeEvent(SDL_Event const& event)
{

}

void Gui::PollEvents()
{
	static SDL_Event event{};
	while (SDL_PollEvent(&event)) {
		ImGui_ImplSDL2_ProcessEvent(&event);
		switch (event.type) {
		case SDL_AUDIODEVICEADDED       : Audio::OnDeviceAdded(event);             break;
		case SDL_AUDIODEVICEREMOVED     : Audio::OnDeviceRemoved(event);           break;
		case SDL_CONTROLLERAXISMOTION   : Input::OnControllerAxisMotion(event);    break;
		case SDL_CONTROLLERBUTTONDOWN   : Input::OnControllerButtonDown(event);    break;
		case SDL_CONTROLLERBUTTONUP     : Input::OnControllerButtonUp(event);      break;
		case SDL_CONTROLLERDEVICEADDED  : Input::OnControllerDeviceAdded(event);   break;
		case SDL_CONTROLLERDEVICEREMOVED: Input::OnControllerDeviceRemoved(event); break;
		case SDL_KEYDOWN                : Input::OnKeyDown(event);                 break;
		case SDL_KEYUP                  : Input::OnKeyUp(event);                   break;
		case SDL_MOUSEBUTTONDOWN        : Input::OnMouseButtonDown(event);         break;
		case SDL_MOUSEBUTTONUP          : Input::OnMouseButtonUp(event);           break;
		case SDL_QUIT                   : OnSdlQuit();                             break;
		case SDL_WINDOWEVENT_RESIZED    : OnWindowResizeEvent(event);              break;
		}
	}
}

void Gui::OnRdpWindowParallelRdpSelected()
{
	// TODO
}

void Gui::OnSdlQuit()
{
	quit = true;
	N64::Stop();
}

bool Gui::ReadConfigFile()
{
	return true; // TODO
}

void Gui::RefreshGameList()
{
	game_list.clear();
	if (fs::exists(game_list_directory)) {
		for (fs::directory_entry const& entry : fs::directory_iterator(game_list_directory)) {
			if (entry.is_regular_file()) {
				auto IsKnownExt = [](fs::path const& ext) {
					static const std::array<fs::path, 8> n64_rom_exts = {
						".n64", ".N64", ".v64", ".V64", ".z64", ".Z64", ".zip", ".7z"
					};
					return std::find_if(n64_rom_exts.begin(), n64_rom_exts.end(), [&ext](fs::path const& known_ext) {
							return ext.compare(known_ext) == 0;
						})
					!= n64_rom_exts.end();
				};
				if (!filter_game_list_to_n64_files || IsKnownExt(entry.path().filename().extension())) {
					game_list.emplace_back(entry.path(), entry.path().filename().string());
				}
			}
		}
	}
}

void Gui::Run(bool boot_game_immediately)
{
	if (boot_game_immediately) {
		start_game = true;
	}
	while (!quit) {
		if (start_game) {
			StartGame();
		}
		else {
			PollEvents();
			N64::UpdateScreen(); // Let the RDP rendering code do its thing (calls back to Gui::Draw())
		}
	}

	StopGame();
}

void Gui::StartGame()
{
	game_is_running = true;
	start_game = false;
	show_game_selection_window = false;
	UpdateWindowTitle();
	N64::Reset();
	N64::Run();
}

void Gui::StopGame()
{
	N64::Stop();
	game_is_running = false;
	UpdateWindowTitle();
	show_game_selection_window = true;
	// TODO: show nice n64 background on window
}

void Gui::TearDown()
{
	StopGame();

	VkResult vk_result = vkDeviceWaitIdle(Vulkan::GetDevice());
	Vulkan::CheckVkResult(vk_result);
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	//ImGui_ImplVulkanH_DestroyWindow(Vulkan::GetInstance(), Vulkan::GetDevice(), &vk_main_window_data, Vulkan::GetAllocator());
	Vulkan::TearDown();

	NFD_Quit();
	SDL_DestroyWindow(sdl_window);
	SDL_Quit();
}

void Gui::UpdateWindowTitle()
{
	if (game_is_running) {
		std::string title = std::format("N63.5 | {} | FPS: {}", current_game_title, fps);
		SDL_SetWindowTitle(sdl_window, title.c_str());
	}
	else {
		SDL_SetWindowTitle(sdl_window, "N63.5");
	}
}

void Gui::UseDefaultConfig()
{
	// TODO
}