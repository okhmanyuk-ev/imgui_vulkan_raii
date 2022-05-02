#pragma once

#include "imgui.h"
#include <vulkan/vulkan_raii.hpp>

struct ImGui_ImplVulkan_InitInfo
{
    vk::Instance Instance;
    VkPhysicalDevice PhysicalDevice;
    vk::Device Device;
    uint32_t QueueFamily;
    VkQueue Queue;
    VkPipelineCache PipelineCache;
    VkDescriptorPool DescriptorPool;
    uint32_t Subpass;
    uint32_t MinImageCount;
    uint32_t ImageCount;
    VkSampleCountFlagBits MSAASamples;
    const VkAllocationCallbacks* Allocator;
    void (*CheckVkResultFn)(VkResult err);
};

IMGUI_IMPL_API bool ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo* info, VkRenderPass render_pass);
IMGUI_IMPL_API void ImGui_ImplVulkan_Shutdown();
IMGUI_IMPL_API void ImGui_ImplVulkan_NewFrame();
IMGUI_IMPL_API void ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, VkCommandBuffer command_buffer, VkPipeline pipeline = VK_NULL_HANDLE);
IMGUI_IMPL_API bool ImGui_ImplVulkan_CreateFontsTexture(VkCommandBuffer command_buffer);
IMGUI_IMPL_API void ImGui_ImplVulkan_DestroyFontUploadObjects();
IMGUI_IMPL_API void ImGui_ImplVulkan_SetMinImageCount(uint32_t min_image_count);

IMGUI_IMPL_API VkDescriptorSet ImGui_ImplVulkan_AddTexture(vk::Sampler sampler, vk::ImageView image_view, vk::ImageLayout image_layout);

struct ImGui_ImplVulkanH_Frame;
struct ImGui_ImplVulkanH_Window;

IMGUI_IMPL_API void ImGui_ImplVulkanH_CreateOrResizeWindow(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, ImGui_ImplVulkanH_Window* wnd, uint32_t queue_family, const VkAllocationCallbacks* allocator, int w, int h, uint32_t min_image_count);
IMGUI_IMPL_API void ImGui_ImplVulkanH_DestroyWindow(vk::Instance instance, vk::Device device, ImGui_ImplVulkanH_Window* wnd, const VkAllocationCallbacks* allocator);
IMGUI_IMPL_API VkSurfaceFormatKHR ImGui_ImplVulkanH_SelectSurfaceFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const VkFormat* request_formats, int request_formats_count, VkColorSpaceKHR request_color_space);
IMGUI_IMPL_API vk::PresentModeKHR ImGui_ImplVulkanH_SelectPresentMode(vk::PhysicalDevice physical_device, VkSurfaceKHR surface, const std::vector<vk::PresentModeKHR>& request_modes);
IMGUI_IMPL_API int ImGui_ImplVulkanH_GetMinImageCountFromPresentMode(vk::PresentModeKHR present_mode);

struct ImGui_ImplVulkanH_Frame
{
    vk::CommandPool CommandPool;
    vk::CommandBuffer CommandBuffer;
    vk::Fence Fence;
    VkImage Backbuffer;
    vk::ImageView BackbufferView;
    VkFramebuffer Framebuffer;
};

struct ImGui_ImplVulkanH_FrameSemaphores
{
    vk::Semaphore ImageAcquiredSemaphore;
    vk::Semaphore RenderCompleteSemaphore;
};

struct ImGui_ImplVulkanH_Window
{
    uint32_t Width;
    uint32_t Height;
    vk::SwapchainKHR Swapchain;
    VkSurfaceKHR Surface;
    vk::SurfaceFormatKHR SurfaceFormat;
    vk::PresentModeKHR PresentMode;
    VkRenderPass RenderPass;
    VkPipeline Pipeline;
    bool ClearEnable;
    vk::ClearValue ClearValue;
    uint32_t FrameIndex;
    uint32_t ImageCount;
    uint32_t SemaphoreIndex;
    ImGui_ImplVulkanH_Frame* Frames;
    ImGui_ImplVulkanH_FrameSemaphores* FrameSemaphores;

    ImGui_ImplVulkanH_Window()
    {
        memset((void*)this, 0, sizeof(*this));
        ClearEnable = true;
    }
};

