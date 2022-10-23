#include <rdp_device.hpp>
#include <wsi.hpp>
#include <volk.h>
#include <vulkan/vulkan.h>

module ParallelRDPWrapper;

import Input;
import RDRAM;
import UserMessage;
import VI;


bool ParallelRDPWrapper::Initialize()
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << "Failed to init SDL video\n";
		return false;
	}
	if (!Vulkan::Context::init_loader(nullptr))
	{
		std::cout << "Failed to initialize Vulkan loader.\n";
		return false;
	}
	sdl_window = SDL_CreateWindow("N63.5",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
		SDL_WINDOW_VULKAN | SDL_WINDOW_INPUT_FOCUS);
	if (!sdl_window) {
		std::cout << "Failed to create SDL window\n";
		return false;
	}

	sdl_wsi_platform = std::make_unique<SDLWSIPlatform>(*this);
	wsi.set_backbuffer_srgb(false);
	wsi.set_platform(sdl_wsi_platform.get());
	wsi.set_present_mode(Vulkan::PresentMode::UnlockedMaybeTear);
	Vulkan::Context::SystemHandles handles{};
	if (!wsi.init_simple(1, handles)) {
		UserMessage::Show("Failed to init wsi.", UserMessage::Type::Error);
		return false;
	}
	wsi_device = &wsi.get_device();

	/* Construct command processor, which we later supply with RDP commands */
	u8* rdram_ptr = RDRAM::GetPointerToMemory();
	size_t rdram_offset{};
	size_t rdram_size = RDRAM::GetSize();
	size_t hidden_rdram_size = rdram_size / 8;
	RDP::CommandProcessorFlags flags{};
	cmd_processor = std::make_unique<RDP::CommandProcessor>(
		*wsi_device, rdram_ptr, rdram_offset, rdram_size, hidden_rdram_size, flags);
	if (!cmd_processor->device_is_supported()) {
		UserMessage::Show("Vulkan device not supported.", UserMessage::Type::Error);
		return false;
	}

	ReloadViRegisters();

	return true;
}


void ParallelRDPWrapper::EnqueueCommand(int cmd_len, u32* cmd_ptr)
{
	cmd_processor->enqueue_command(cmd_len, cmd_ptr);
}


void ParallelRDPWrapper::OnFullSync()
{
	cmd_processor->wait_for_timeline(cmd_processor->signal_timeline());
}


void ParallelRDPWrapper::ReloadViRegisters()
{
	/* TODO: only call set_vi_register when a VI register is actually written to, if that does not lead to race conditions */
	const VI::Registers& vi = VI::ReadAllRegisters();
	cmd_processor->set_vi_register(RDP::VIRegister::Control, vi.ctrl);
	cmd_processor->set_vi_register(RDP::VIRegister::Origin, vi.origin);
	cmd_processor->set_vi_register(RDP::VIRegister::Width, vi.width);
	cmd_processor->set_vi_register(RDP::VIRegister::Intr, vi.v_intr);
	cmd_processor->set_vi_register(RDP::VIRegister::VCurrentLine, vi.v_current);
	cmd_processor->set_vi_register(RDP::VIRegister::Timing, vi.burst);
	cmd_processor->set_vi_register(RDP::VIRegister::VSync, vi.v_sync);
	cmd_processor->set_vi_register(RDP::VIRegister::HSync, vi.h_sync);
	cmd_processor->set_vi_register(RDP::VIRegister::Leap, vi.h_sync_leap);
	cmd_processor->set_vi_register(RDP::VIRegister::HStart, vi.h_video);
	cmd_processor->set_vi_register(RDP::VIRegister::VStart, vi.v_video);
	cmd_processor->set_vi_register(RDP::VIRegister::VBurst, vi.v_burst);
	cmd_processor->set_vi_register(RDP::VIRegister::XScale, vi.x_scale);
	cmd_processor->set_vi_register(RDP::VIRegister::YScale, vi.y_scale);
}


void ParallelRDPWrapper::TearDown()
{
	cmd_processor.reset();
	wsi.teardown();
	SDL_DestroyWindow(sdl_window);
	sdl_window = nullptr;
}


void ParallelRDPWrapper::UpdateScreen()
{
	ReloadViRegisters();

	wsi.begin_frame();
	Vulkan::ImageHandle image = cmd_processor->scanout(scanout_opts);

	// Normally reflection is automated.
	Vulkan::ResourceLayout vertex_layout = {};
	Vulkan::ResourceLayout fragment_layout = {};
	fragment_layout.output_mask = 1 << 0;
	fragment_layout.sets[0].sampled_image_mask = 1 << 0;

	// This request is cached.
	auto* program = wsi_device->request_program(vertex_spirv, sizeof(vertex_spirv),
		fragment_spirv, sizeof(fragment_spirv),
		&vertex_layout,
		&fragment_layout);

	// Blit image on screen.
	auto cmd = wsi_device->request_command_buffer();
	{
		auto rp = wsi_device->get_swapchain_render_pass(Vulkan::SwapchainRenderPass::ColorOnly);
		cmd->begin_render_pass(rp);

		VkViewport vp = cmd->get_viewport();
		// Adjust the viewport here for aspect ratio correction.

		cmd->set_program(program);

		// Basic default render state.
		cmd->set_opaque_state();
		cmd->set_depth_test(false, false);
		cmd->set_cull_mode(VK_CULL_MODE_NONE);

		// If we don't have an image, we just get a cleared screen in the render pass.
		if (image)
		{
			cmd->set_texture(0, 0, image->get_view(), Vulkan::StockSampler::LinearClamp);
			cmd->set_viewport(vp);
			// The vertices are constants in the shader.
			// Draws fullscreen quad using oversized triangle.
			cmd->draw(3);
		}

		cmd->end_render_pass();
	}
	wsi_device->submit(cmd);
	wsi.end_frame();
}


bool ParallelRDPWrapper::SDLWSIPlatform::alive(Vulkan::WSI& wsi)
{
	return true;
}


VkSurfaceKHR ParallelRDPWrapper::SDLWSIPlatform::create_surface(VkInstance instance, VkPhysicalDevice gpu)
{
	VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
	if (!SDL_Vulkan_CreateSurface(parallel_rdp_wrapper.sdl_window, instance, &vk_surface)) {
		UserMessage::Show(std::format("Failed to create Vulkan surface: {}", SDL_GetError()), UserMessage::Type::Error);
		return VK_NULL_HANDLE;
	}
	return vk_surface;
}


void ParallelRDPWrapper::SDLWSIPlatform::event_frame_tick(double frame, double elapsed)
{

}


const VkApplicationInfo* ParallelRDPWrapper::SDLWSIPlatform::get_application_info()
{
	static constexpr VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.apiVersion = VK_API_VERSION_1_1
	};
	return &app_info;
}


std::vector<const char*> ParallelRDPWrapper::SDLWSIPlatform::get_instance_extensions()
{
	uint num_extensions;
	if (!SDL_Vulkan_GetInstanceExtensions(parallel_rdp_wrapper.sdl_window, &num_extensions, nullptr)) {
		UserMessage::Show("Failed to get Vulkan instance extensions.", UserMessage::Type::Error);
		return {};
	}
	std::vector<const char*> extensions(num_extensions);
	if (!SDL_Vulkan_GetInstanceExtensions(parallel_rdp_wrapper.sdl_window, &num_extensions, extensions.data())) {
		UserMessage::Show("Failed to get Vulkan instance extensions.", UserMessage::Type::Error);
		return {};
	}
	return extensions;
}


u32 ParallelRDPWrapper::SDLWSIPlatform::get_surface_width()
{
	return 640;
}


u32 ParallelRDPWrapper::SDLWSIPlatform::get_surface_height()
{
	return 480;
}


void ParallelRDPWrapper::SDLWSIPlatform::poll_input()
{
	Input::Poll();
}