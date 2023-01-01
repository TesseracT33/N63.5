module;

#include "imgui_impl_vulkan.h"
#include "vulkan/vulkan.h"

module Vulkan;

import ParallelRDPWrapper;
import RDP;
import RDPImplementation;
import UserMessage;

void Vulkan::CheckVkResult(VkResult vk_result)
{
	if (vk_result != 0) {
		std::cerr << "[vulkan]: Error: VkResult = " << std::to_underlying(vk_result) << '\n';
		if (vk_result < 0) {
			exit(1);
		}
	}
}

// Currently unused. To be used in the future if I ever write my own RDP implementation using vulkan
void Vulkan::FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
{
#if 0
	VkResult vk_result;

	VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
	VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
	vk_result = vkAcquireNextImageKHR(vk_device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
	if (vk_result == VK_ERROR_OUT_OF_DATE_KHR || vk_result == VK_SUBOPTIMAL_KHR) {
		vk_swap_chain_rebuild = true;
		return;
	}
	CheckVkResult(vk_result);

	ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
	{
		vk_result = vkWaitForFences(vk_device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
		CheckVkResult(vk_result);
		vk_result = vkResetFences(vk_device, 1, &fd->Fence);
		CheckVkResult(vk_result);
	}
	{
		vk_result = vkResetCommandPool(vk_device, fd->CommandPool, 0);
		CheckVkResult(vk_result);
		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vk_result = vkBeginCommandBuffer(fd->CommandBuffer, &info);
		CheckVkResult(vk_result);
	}
	{
		VkRenderPassBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.renderPass = wd->RenderPass;
		info.framebuffer = fd->Framebuffer;
		info.renderArea.extent.width = wd->Width;
		info.renderArea.extent.height = wd->Height;
		info.clearValueCount = 1;
		info.pClearValues = &wd->ClearValue;
		vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
	}

	// Record dear imgui primitives into command buffer
	ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

	// Submit command buffer
	vkCmdEndRenderPass(fd->CommandBuffer);
	{
		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &image_acquired_semaphore;
		info.pWaitDstStageMask = &wait_stage;
		info.commandBufferCount = 1;
		info.pCommandBuffers = &fd->CommandBuffer;
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &render_complete_semaphore;

		vk_result = vkEndCommandBuffer(fd->CommandBuffer);
		CheckVkResult(vk_result);
		vk_result = vkQueueSubmit(vk_queue, 1, &info, fd->Fence);
		CheckVkResult(vk_result);
	}
#endif
}

// Currently unused. To be used in the future if I ever write my own RDP implementation using vulkan
void Vulkan::FramePresent(ImGui_ImplVulkanH_Window* wd)
{
#if 0
	if (vk_swap_chain_rebuild) {
		return;
	}
	VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
	VkPresentInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &render_complete_semaphore;
	info.swapchainCount = 1;
	info.pSwapchains = &wd->Swapchain;
	info.pImageIndices = &wd->FrameIndex;
	VkResult vk_result = vkQueuePresentKHR(vk_queue, &info);
	if (vk_result == VK_ERROR_OUT_OF_DATE_KHR || vk_result == VK_SUBOPTIMAL_KHR) {
		vk_swap_chain_rebuild = true;
		return;
	}
	CheckVkResult(vk_result);
	wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount; // Now we can use the next set of semaphores
#endif
}

VkAllocationCallbacks* Vulkan::GetAllocator()
{
	return vk_allocator;
}

VkCommandBuffer Vulkan::GetCommandBuffer()
{
	return vk_command_buffer;
}

VkDescriptorPool Vulkan::GetDescriptorPool()
{
	return vk_descriptor_pool;
}

VkDevice Vulkan::GetDevice()
{
	return vk_device;
}

VkFormat Vulkan::GetFormat()
{
	return vk_format;
}

u32 Vulkan::GetGraphicsQueueFamily()
{
	return vk_queue_family;
}

VkInstance Vulkan::GetInstance()
{
	return vk_instance;
}

VkPipelineCache Vulkan::GetPipelineCache()
{
	return vk_pipeline_cache;
}

VkPhysicalDevice Vulkan::GetPhysicalDevice()
{
	return vk_physical_device;
}

VkQueue Vulkan::GetQueue()
{
	return vk_queue;
}

VkRenderPass Vulkan::GetRenderPass()
{
	return vk_render_pass;
}

bool Vulkan::Init([[maybe_unused]] SDL_Window* sdl_window)
{
	return InitForParallelRDP();
}

bool Vulkan::InitForParallelRDP()
{ // TODO: possibly put this in ParallelRDPWrapper class instead
	if (!RDP::implementation) {
		if (!RDP::MakeParallelRdp() || !RDP::implementation) {
			UserMessage::Error("Failed to initialize vulkan for ParallelRDP");
			return false;
		}
	}
	ParallelRDPWrapper* const parallel_rdp = dynamic_cast<ParallelRDPWrapper*>(RDP::implementation.get());
	assert(parallel_rdp);

	vk_instance = parallel_rdp->GetVkInstance();
	vk_physical_device = parallel_rdp->GetVkPhysicalDevice();
	vk_device = parallel_rdp->GetVkDevice();
	vk_queue_family = parallel_rdp->GetVkQueueFamily();
	vk_queue = parallel_rdp->GetVkQueue();
	vk_pipeline_cache = {};
	vk_descriptor_pool = {};
	vk_allocator = {};
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
		pool_info.maxSets = 1000 * (u32)std::size(pool_sizes);
		pool_info.poolSizeCount = (u32)std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		vk_result = vkCreateDescriptorPool(vk_device, &pool_info, vk_allocator, &vk_descriptor_pool);
		CheckVkResult(vk_result);
	}

	{ // Create the Render Pass
		VkAttachmentDescription attachment = {};
		attachment.format = vk_format;
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

	return true;
}

// Currently unused. To be used in the future if I ever write my own RDP implementation using vulkan
bool Vulkan::InitGeneric(SDL_Window* sdl_window)
{
#if 0
	if (!sdl_window) {
		std::cerr << "[vulkan] nullptr SDL_Window passed to Vulkan::InitGeneric\n";
		return false;
	}

	VkResult vk_result;

	u32 extensions_count{};
	SDL_Vulkan_GetInstanceExtensions(sdl_window, &extensions_count, nullptr);
	std::vector<const char*> extensions(extensions_count);
	SDL_Vulkan_GetInstanceExtensions(sdl_window, &extensions_count, extensions.data());

	{ // Create Vulkan Instance
		VkInstanceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.enabledExtensionCount = extensions_count;
		create_info.ppEnabledExtensionNames = extensions.data();
		// Create Vulkan Instance without any debug feature
		vk_result = vkCreateInstance(&create_info, vk_allocator, &vk_instance);
		CheckVkResult(vk_result);
	}

	{ // Select GPU
		u32 gpu_count;
		vk_result = vkEnumeratePhysicalDevices(vk_instance, &gpu_count, nullptr);
		CheckVkResult(vk_result);
		IM_ASSERT(gpu_count > 0);

		std::vector<VkPhysicalDevice> gpus(gpu_count);
		vk_result = vkEnumeratePhysicalDevices(vk_instance, &gpu_count, gpus.data());
		CheckVkResult(vk_result);

		// If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers
		// most common cases (multi-gpu/integrated+dedicated graphics). Handling more complicated setups (multiple
		// dedicated GPUs) is out of scope of this sample.
		u32 use_gpu = 0;
		for (u32 i = 0; i < gpu_count; ++i) {
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(gpus[i], &properties);
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				use_gpu = i;
				break;
			}
		}

		vk_physical_device = gpus[use_gpu];
	}

	{ // Select graphics queue family
		u32 count;
		vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &count, nullptr);
		std::vector<VkQueueFamilyProperties> queues(count);
		vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &count, queues.data());
		for (u32 i = 0; i < count; ++i) {
			if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				vk_queue_family = i;
				break;
			}
		}
		IM_ASSERT(vk_queue_family != u32(-1));
	}

	{ // Create Logical Device (with 1 queue)
		int device_extension_count = 1;
		const char* device_extensions[] = { "VK_KHR_swapchain" };
		const float queue_priority[] = { 1.0f };
		VkDeviceQueueCreateInfo queue_info[1] = {};
		queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info[0].queueFamilyIndex = vk_queue_family;
		queue_info[0].queueCount = 1;
		queue_info[0].pQueuePriorities = queue_priority;
		VkDeviceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
		create_info.pQueueCreateInfos = queue_info;
		create_info.enabledExtensionCount = device_extension_count;
		create_info.ppEnabledExtensionNames = device_extensions;
		vk_result = vkCreateDevice(vk_physical_device, &create_info, vk_allocator, &vk_device);
		CheckVkResult(vk_result);
		vkGetDeviceQueue(vk_device, vk_queue_family, 0, &vk_queue);
	}

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
		pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		vk_result = vkCreateDescriptorPool(vk_device, &pool_info, vk_allocator, &vk_descriptor_pool);
		CheckVkResult(vk_result);
	}

	// Create Window Surface
	VkSurfaceKHR surface;
	if (SDL_Vulkan_CreateSurface(sdl_window, vk_instance, &surface) == 0) {
		UserMessage::Fatal("[vulkan] Failed to create Vulkan surface.");
		return false;
	}

	// Create Framebuffers
	int w, h;
	SDL_GetWindowSize(sdl_window, &w, &h);
	ImGui_ImplVulkanH_Window* wd = &vk_main_window_data;
	wd->Surface = surface;
	// Check for WSI support
	VkBool32 res;
	vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device, vk_queue_family, wd->Surface, &res);
	if (res != VK_TRUE) {
		UserMessage::Fatal("[vulkan] No WSI support on physical device 0");
		exit(-1);
	}

	// Select Surface Format
	const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
	const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(vk_physical_device, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

	// Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
	VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
	VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
	wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(vk_physical_device, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
	//printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

	// Create SwapChain, RenderPass, Framebuffer, etc.
	IM_ASSERT(vk_min_image_count >= 2);
	ImGui_ImplVulkanH_CreateOrResizeWindow(vk_instance, vk_physical_device, vk_device, wd, vk_queue_family, vk_allocator, w, h, vk_min_image_count);
#endif

	return true;
}

void Vulkan::SubmitRequestedCommandBuffer()
{
	// wsi->get_device().submit(requested_command_buffer); TODO
}

void Vulkan::TearDown()
{
	vkDestroyDescriptorPool(vk_device, vk_descriptor_pool, vk_allocator);
	vkDestroyDevice(vk_device, vk_allocator);
	vkDestroyInstance(vk_instance, vk_allocator);

	vk_allocator = {};
	vk_command_buffer = {};
	vk_descriptor_pool = {};
	vk_device = {};
	vk_format = {};
	vk_instance = {};
	vk_min_image_count = {};
	vk_physical_device = {};
	vk_pipeline_cache = {};
	vk_queue = {};
	vk_queue_family = {};
	vk_render_pass = {};
	vk_swap_chain_rebuild = {};
}