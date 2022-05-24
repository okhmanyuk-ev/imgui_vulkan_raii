#pragma once

#include "imgui.h"
#include <vulkan/vulkan_raii.hpp>

inline vk::raii::Context gContext;
inline vk::raii::Instance g_Instance = nullptr;
inline vk::raii::PhysicalDevice g_PhysicalDevice = nullptr;
inline vk::raii::Device g_Device = nullptr;
inline uint32_t g_QueueFamily = (uint32_t)-1;
inline vk::raii::Queue g_Queue = nullptr;
inline vk::raii::PipelineCache g_PipelineCache = nullptr;
inline vk::raii::DescriptorPool g_DescriptorPool = nullptr;

struct ImGui_ImplVulkan_InitInfo
{
	uint32_t MinImageCount;
	uint32_t ImageCount;
};

IMGUI_IMPL_API bool ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo* info);
IMGUI_IMPL_API void ImGui_ImplVulkan_Shutdown();
IMGUI_IMPL_API void ImGui_ImplVulkan_NewFrame();
IMGUI_IMPL_API void ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, vk::raii::CommandBuffer& command_buffer);
IMGUI_IMPL_API bool ImGui_ImplVulkan_CreateFontsTexture(vk::raii::CommandBuffer& command_buffer);
IMGUI_IMPL_API void ImGui_ImplVulkan_SetMinImageCount(uint32_t min_image_count);

IMGUI_IMPL_API vk::raii::DescriptorSet ImGui_ImplVulkan_AddTexture(vk::Sampler sampler, vk::ImageView image_view, vk::ImageLayout image_layout);

struct ImGui_ImplVulkanH_Frame;
struct ImGui_ImplVulkanH_Window;

IMGUI_IMPL_API void ImGui_ImplVulkanH_CreateOrResizeWindow(vk::Instance instance, vk::PhysicalDevice physical_device, vk::raii::Device& device, ImGui_ImplVulkanH_Window& wnd, uint32_t queue_family, int w, int h, uint32_t min_image_count);
IMGUI_IMPL_API void ImGui_ImplVulkanH_DestroyWindow(vk::Instance instance, vk::Device device, ImGui_ImplVulkanH_Window& wnd);
IMGUI_IMPL_API vk::SurfaceFormatKHR ImGui_ImplVulkanH_SelectSurfaceFormat(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, std::vector<vk::Format> request_formats, vk::ColorSpaceKHR request_color_space);
IMGUI_IMPL_API vk::PresentModeKHR ImGui_ImplVulkanH_SelectPresentMode(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, const std::vector<vk::PresentModeKHR>& request_modes);
IMGUI_IMPL_API int ImGui_ImplVulkanH_GetMinImageCountFromPresentMode(vk::PresentModeKHR present_mode);

struct ImGui_ImplVulkanH_Frame
{
	vk::raii::CommandPool CommandPool = nullptr;
	vk::raii::CommandBuffer CommandBuffer = nullptr;
	vk::raii::Fence Fence = nullptr;
	vk::Image Backbuffer;
	vk::raii::ImageView BackbufferView = nullptr;
	vk::raii::Semaphore ImageAcquiredSemaphore = nullptr;
	vk::raii::Semaphore RenderCompleteSemaphore = nullptr;
};

struct ImGui_ImplVulkanH_Window
{
	uint32_t Width;
	uint32_t Height;
	vk::raii::SwapchainKHR Swapchain = nullptr;
	vk::raii::SurfaceKHR Surface = nullptr;
	vk::SurfaceFormatKHR SurfaceFormat;
	vk::PresentModeKHR PresentMode;
	vk::ClearValue ClearValue;
	uint32_t FrameIndex;
	uint32_t SemaphoreIndex;
	std::vector<ImGui_ImplVulkanH_Frame> Frames;
};

inline ImGui_ImplVulkanH_Window g_MainWindowData;
