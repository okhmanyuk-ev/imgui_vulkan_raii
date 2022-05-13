#pragma once

#include "imgui.h"
#include <vulkan/vulkan_raii.hpp>

struct ImGui_ImplVulkan_InitInfo
{
	vk::Instance Instance;
	vk::PhysicalDevice PhysicalDevice;
	vk::Device Device;
	uint32_t QueueFamily;
	vk::Queue Queue;
	vk::PipelineCache PipelineCache;
	vk::DescriptorPool DescriptorPool;
	uint32_t Subpass;
	uint32_t MinImageCount;
	uint32_t ImageCount;
};

IMGUI_IMPL_API bool ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo* info, vk::RenderPass render_pass);
IMGUI_IMPL_API void ImGui_ImplVulkan_Shutdown();
IMGUI_IMPL_API void ImGui_ImplVulkan_NewFrame();
IMGUI_IMPL_API void ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, vk::CommandBuffer command_buffer, vk::Pipeline pipeline = VK_NULL_HANDLE);
IMGUI_IMPL_API bool ImGui_ImplVulkan_CreateFontsTexture(vk::CommandBuffer command_buffer);
IMGUI_IMPL_API void ImGui_ImplVulkan_DestroyFontUploadObjects();
IMGUI_IMPL_API void ImGui_ImplVulkan_SetMinImageCount(uint32_t min_image_count);

IMGUI_IMPL_API vk::DescriptorSet ImGui_ImplVulkan_AddTexture(vk::Sampler sampler, vk::ImageView image_view, vk::ImageLayout image_layout);

struct ImGui_ImplVulkanH_Frame;
struct ImGui_ImplVulkanH_Window;

IMGUI_IMPL_API void ImGui_ImplVulkanH_CreateOrResizeWindow(vk::Instance instance, vk::PhysicalDevice physical_device, vk::Device device, ImGui_ImplVulkanH_Window& wnd, uint32_t queue_family, int w, int h, uint32_t min_image_count);
IMGUI_IMPL_API void ImGui_ImplVulkanH_DestroyWindow(vk::Instance instance, vk::Device device, ImGui_ImplVulkanH_Window& wnd);
IMGUI_IMPL_API vk::SurfaceFormatKHR ImGui_ImplVulkanH_SelectSurfaceFormat(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, std::vector<vk::Format> request_formats, vk::ColorSpaceKHR request_color_space);
IMGUI_IMPL_API vk::PresentModeKHR ImGui_ImplVulkanH_SelectPresentMode(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, const std::vector<vk::PresentModeKHR>& request_modes);
IMGUI_IMPL_API int ImGui_ImplVulkanH_GetMinImageCountFromPresentMode(vk::PresentModeKHR present_mode);

struct ImGui_ImplVulkanH_Frame
{
	vk::CommandPool CommandPool;
	vk::CommandBuffer CommandBuffer;
	vk::Fence Fence;
	vk::Image Backbuffer;
	vk::ImageView BackbufferView;
	vk::Framebuffer Framebuffer;
	vk::Semaphore ImageAcquiredSemaphore;
	vk::Semaphore RenderCompleteSemaphore;
};

struct ImGui_ImplVulkanH_Window
{
	uint32_t Width;
	uint32_t Height;
	vk::SwapchainKHR Swapchain;
	vk::SurfaceKHR Surface;
	vk::SurfaceFormatKHR SurfaceFormat;
	vk::PresentModeKHR PresentMode;
	vk::RenderPass RenderPass;
	vk::Pipeline Pipeline;
	vk::ClearValue ClearValue;
	uint32_t FrameIndex;
	uint32_t SemaphoreIndex;
	std::vector<ImGui_ImplVulkanH_Frame> Frames;
};

