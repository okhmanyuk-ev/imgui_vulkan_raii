// Dear ImGui: standalone example application for Glfw + Vulkan
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// Important note to the reader who wish to integrate imgui_impl_vulkan.cpp/.h in their own engine/app.
// - Common ImGui_ImplVulkan_XXX functions and structures are used to interface with imgui_impl_vulkan.cpp/.h.
//   You will use those if you want to use this rendering backend in your engine/app.
// - Helper ImGui_ImplVulkanH_XXX functions and structures are only used by this example (main.cpp) and by
//   the backend itself (imgui_impl_vulkan.cpp), but should PROBABLY NOT be used by your own engine/app code.
// Read comments in imgui_impl_vulkan.h.

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

static ImGui_ImplVulkanH_Window g_MainWindowData;
static int g_MinImageCount = 2;
static bool g_SwapChainRebuild = false;

static void SetupVulkan(const char** extensions, uint32_t extensions_count)
{
	auto layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	auto application_info = vk::ApplicationInfo()
		.setApiVersion(VK_API_VERSION_1_3);

	auto instance_create_info = vk::InstanceCreateInfo()
		.setEnabledExtensionCount(extensions_count)
		.setPpEnabledExtensionNames(extensions)
		.setPEnabledLayerNames(layers)
		.setPApplicationInfo(&application_info);

	g_Instance = gContext.createInstance(instance_create_info);
	
	auto gpus = g_Instance.enumeratePhysicalDevices();
	int use_gpu = 0;
	for (int i = 0; i < (int)gpus.size(); i++)
	{
		auto properties = gpus[i].getProperties();			
		if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		{
			use_gpu = i;
			break;
		}
	}

	g_PhysicalDevice = std::move(gpus.at(use_gpu));
	
	auto queues = g_PhysicalDevice.getQueueFamilyProperties();

	for (uint32_t i = 0; i < queues.size(); i++)
	{
		if (queues[i].queueFlags & vk::QueueFlagBits::eGraphics)
		{
			g_QueueFamily = i;
			break;
		}
	}
	
	IM_ASSERT(g_QueueFamily != (uint32_t)-1);
	
	auto device_extensions = { 
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	auto queue_priority = { 1.0f };

	auto device_queue_create_info = vk::DeviceQueueCreateInfo()
		.setQueueFamilyIndex(g_QueueFamily)
		.setQueuePriorities(queue_priority);

	auto device_features = g_PhysicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features>();

	auto device_create_info = vk::DeviceCreateInfo()
		.setQueueCreateInfoCount(1)
		.setPQueueCreateInfos(&device_queue_create_info)
		.setPEnabledExtensionNames(device_extensions)
		.setPEnabledFeatures(nullptr)
		.setPNext(&device_features);
		
	g_Device = g_PhysicalDevice.createDevice(device_create_info);
	g_Queue = g_Device.getQueue(g_QueueFamily, 0);
	
	std::vector<vk::DescriptorPoolSize> pool_sizes = {
		{ vk::DescriptorType::eSampler, 1000 },
		{ vk::DescriptorType::eCombinedImageSampler, 1000 },
		{ vk::DescriptorType::eSampledImage, 1000 },
		{ vk::DescriptorType::eStorageImage, 1000 },
		{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
		{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
		{ vk::DescriptorType::eUniformBuffer, 1000 },
		{ vk::DescriptorType::eStorageBuffer, 1000 },
		{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
		{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
		{ vk::DescriptorType::eInputAttachment, 1000 }
	};
		
	auto descriptor_pool_create_info = vk::DescriptorPoolCreateInfo()
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
		.setMaxSets(uint32_t(1000 * pool_sizes.size()))
		.setPoolSizes(pool_sizes);

	g_DescriptorPool = g_Device.createDescriptorPool(descriptor_pool_create_info);
}

static void SetupVulkanWindow(ImGui_ImplVulkanH_Window& wd, vk::SurfaceKHR surface, int width, int height)
{
	wd.Surface = vk::raii::SurfaceKHR(g_Instance, surface);

	auto requestSurfaceImageFormat = { 
		vk::Format::eB8G8R8A8Unorm,
		vk::Format::eR8G8B8A8Unorm,
		vk::Format::eB8G8R8Unorm,
		vk::Format::eR8G8B8Unorm
	};

	auto requestSurfaceColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	
	wd.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(*g_PhysicalDevice, *wd.Surface, requestSurfaceImageFormat, requestSurfaceColorSpace);

	auto present_modes = { vk::PresentModeKHR::eFifo };
	wd.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(*g_PhysicalDevice, *wd.Surface, present_modes);
	
	IM_ASSERT(g_MinImageCount >= 2);
	ImGui_ImplVulkanH_CreateOrResizeWindow(*g_Instance, *g_PhysicalDevice, g_Device, wd, g_QueueFamily, width, height, g_MinImageCount);
}

static void CleanupVulkanWindow()
{
	ImGui_ImplVulkanH_DestroyWindow(*g_Instance, *g_Device, g_MainWindowData);
}

static void FrameRender(ImGui_ImplVulkanH_Window& wd, ImDrawData* draw_data)
{
	auto& image_acquired_semaphore = wd.Frames[wd.SemaphoreIndex].ImageAcquiredSemaphore;
	auto& render_complete_semaphore = wd.Frames[wd.SemaphoreIndex].RenderCompleteSemaphore;

	auto [result, frame_index] = wd.Swapchain.acquireNextImage(UINT64_MAX, *image_acquired_semaphore);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
	{
		g_SwapChainRebuild = true;
		return;
	}

	wd.FrameIndex = frame_index;

	auto& fd = wd.Frames[wd.FrameIndex];
	
	g_Device.waitForFences({ *fd.Fence }, true, UINT64_MAX);
	g_Device.resetFences({ *fd.Fence });
	
	fd.CommandPool.reset();
	
	auto command_buffer_begin_info = vk::CommandBufferBeginInfo()
		.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	fd.CommandBuffer.begin(command_buffer_begin_info);
	
	auto render_pass_begin_info = vk::RenderPassBeginInfo()
		.setRenderPass(*g_RenderPass)
		.setFramebuffer(*fd.Framebuffer)
		.setRenderArea({ { 0, 0 }, { wd.Width, wd.Height } })
		.setClearValueCount(1)
		.setPClearValues(&wd.ClearValue);

	fd.CommandBuffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
	
	ImGui_ImplVulkan_RenderDrawData(draw_data, fd.CommandBuffer);

	fd.CommandBuffer.endRenderPass();
	fd.CommandBuffer.end();
	
	vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	auto submit_info = vk::SubmitInfo()
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&*image_acquired_semaphore)
		.setPWaitDstStageMask(&wait_dst_stage_mask)
		.setCommandBufferCount(1)
		.setPCommandBuffers(&*fd.CommandBuffer)
		.setSignalSemaphoreCount(1)
		.setPSignalSemaphores(&*render_complete_semaphore);

	g_Queue.submit({ submit_info }, *fd.Fence);
}

static void FramePresent(ImGui_ImplVulkanH_Window& wd)
{
	if (g_SwapChainRebuild)
		return;
	
	auto& render_complete_semaphore = wd.Frames[wd.SemaphoreIndex].RenderCompleteSemaphore;

	auto present_info = vk::PresentInfoKHR()
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&*render_complete_semaphore)
		.setSwapchainCount(1)
		.setPSwapchains(&*wd.Swapchain)
		.setPImageIndices(&wd.FrameIndex);

	auto result = g_Queue.presentKHR(present_info); // TODO: crash when resizing
	
	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
	{
		g_SwapChainRebuild = true;
		return;
	}

	wd.SemaphoreIndex = (wd.SemaphoreIndex + 1) % wd.Frames.size();
}

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**)
{
	// Setup GLFW window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+Vulkan example", NULL, NULL);

	// Setup Vulkan
	if (!glfwVulkanSupported())
	{
		printf("GLFW: Vulkan Not Supported\n");
		return 1;
	}
	uint32_t extensions_count = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
	SetupVulkan(extensions, extensions_count);

	// Create Window Surface
	VkSurfaceKHR surface;
	glfwCreateWindowSurface(*g_Instance, window, nullptr, &surface);
	
	// Create Framebuffers
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	auto& wd = g_MainWindowData;
	SetupVulkanWindow(wd, surface, w, h);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	auto& io = ImGui::GetIO(); (void)io;
	
	ImGui::StyleColorsClassic();
	
	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Subpass = 0;
	init_info.MinImageCount = g_MinImageCount;
	init_info.ImageCount = (uint32_t)wd.Frames.size();
	ImGui_ImplVulkan_Init(&init_info);

	{
		auto& command_pool = wd.Frames[wd.FrameIndex].CommandPool;
		auto& command_buffer = wd.Frames[wd.FrameIndex].CommandBuffer;

		command_pool.reset();

		auto command_buffer_begin_info = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		command_buffer.begin(command_buffer_begin_info);

		ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

		command_buffer.end();

		auto submit_info = vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&*command_buffer);

		g_Queue.submit({ submit_info });

		g_Device.waitIdle();
	}

	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		if (g_SwapChainRebuild)
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			if (width > 0 && height > 0)
			{
				ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
				ImGui_ImplVulkanH_CreateOrResizeWindow(*g_Instance, *g_PhysicalDevice, g_Device, g_MainWindowData, g_QueueFamily, width, height, g_MinImageCount);
				g_MainWindowData.FrameIndex = 0;
				g_SwapChainRebuild = false;
			}
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		auto draw_data = ImGui::GetDrawData();
		const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
		if (!is_minimized)
		{
			wd.ClearValue.color.float32[0] = clear_color.x * clear_color.w;
			wd.ClearValue.color.float32[1] = clear_color.y * clear_color.w;
			wd.ClearValue.color.float32[2] = clear_color.z * clear_color.w;
			wd.ClearValue.color.float32[3] = clear_color.w;
			FrameRender(wd, draw_data);
			FramePresent(wd);
		}
	}

	g_Device.waitIdle();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	CleanupVulkanWindow();
	
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
