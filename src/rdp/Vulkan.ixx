module;

#include "imgui_impl_vulkan.h"
#include "vulkan/vulkan.h"

export module Vulkan;

import Util;

import "SDL.h";
import "SDL_vulkan.h";

import <iostream>;
import <utility>;
import <vector>;

namespace Vulkan
{
	export {
		void CheckVkResult(VkResult vk_result);
		void FramePresent(ImGui_ImplVulkanH_Window* wd);
		void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data);
		VkAllocationCallbacks* GetAllocator();
		VkCommandBuffer GetCommandBuffer();
		VkDescriptorPool GetDescriptorPool();
		VkDevice GetDevice();
		VkFormat GetFormat();
		u32 GetGraphicsQueueFamily();
		VkInstance GetInstance();
		VkPhysicalDevice GetPhysicalDevice();
		VkPipelineCache GetPipelineCache();
		VkQueue GetQueue();
		VkRenderPass GetRenderPass();
		bool Init(SDL_Window* sdl_window);
		bool InitForParallelRDP();
		bool InitGeneric(SDL_Window* sdl_window);
		void SubmitRequestedCommandBuffer();
		void TearDown();
	}

	VkAllocationCallbacks* vk_allocator;
	VkCommandBuffer        vk_command_buffer;
	VkDescriptorPool       vk_descriptor_pool;
	VkDevice               vk_device;
	VkFormat               vk_format;
	VkInstance             vk_instance;
	u32                    vk_min_image_count;
	VkPhysicalDevice       vk_physical_device;
	VkPipelineCache        vk_pipeline_cache;
	VkQueue                vk_queue;
	u32                    vk_queue_family;
	VkRenderPass           vk_render_pass;
	bool                   vk_swap_chain_rebuild;
}