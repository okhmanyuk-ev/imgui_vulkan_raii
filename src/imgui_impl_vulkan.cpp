#include "imgui_impl_vulkan.h"
#include <stdio.h>

struct ImGui_ImplVulkanH_FrameRenderBuffers
{
	VkDeviceMemory VertexBufferMemory;
	VkDeviceMemory IndexBufferMemory;
	VkDeviceSize VertexBufferSize;
	VkDeviceSize IndexBufferSize;
	vk::Buffer VertexBuffer;
	vk::Buffer IndexBuffer;
};

struct ImGui_ImplVulkanH_WindowRenderBuffers
{
	uint32_t            Index;
	uint32_t            Count;
	ImGui_ImplVulkanH_FrameRenderBuffers*   FrameRenderBuffers;
};

struct ImGui_ImplVulkan_Data
{
	ImGui_ImplVulkan_InitInfo VulkanInitInfo;
	VkRenderPass RenderPass;
	VkDeviceSize BufferMemoryAlignment;
	VkPipelineCreateFlags PipelineCreateFlags;
	vk::DescriptorSetLayout DescriptorSetLayout;
	vk::PipelineLayout PipelineLayout;
	VkPipeline Pipeline;
	uint32_t Subpass;
	VkShaderModule ShaderModuleVert;
	VkShaderModule ShaderModuleFrag;

	// Font data
	vk::Sampler FontSampler;
	VkDeviceMemory FontMemory;
	VkImage FontImage;
	VkImageView FontView;
	VkDescriptorSet FontDescriptorSet;
	VkDeviceMemory UploadBufferMemory;
	VkBuffer UploadBuffer;

	// Render buffers
	ImGui_ImplVulkanH_WindowRenderBuffers MainWindowRenderBuffers;

	ImGui_ImplVulkan_Data()
	{
		memset((void*)this, 0, sizeof(*this));
		BufferMemoryAlignment = 256;
	}
};

// Forward Declarations
bool ImGui_ImplVulkan_CreateDeviceObjects();
void ImGui_ImplVulkan_DestroyDeviceObjects();
void ImGui_ImplVulkanH_DestroyFrame(vk::Device device, ImGui_ImplVulkanH_Frame* fd, const VkAllocationCallbacks* allocator);
void ImGui_ImplVulkanH_DestroyFrameSemaphores(vk::Device device, ImGui_ImplVulkanH_FrameSemaphores* fsd, const VkAllocationCallbacks* allocator);
void ImGui_ImplVulkanH_DestroyFrameRenderBuffers(vk::Device device, ImGui_ImplVulkanH_FrameRenderBuffers* buffers, const VkAllocationCallbacks* allocator);
void ImGui_ImplVulkanH_DestroyWindowRenderBuffers(VkDevice device, ImGui_ImplVulkanH_WindowRenderBuffers* buffers, const VkAllocationCallbacks* allocator);
void ImGui_ImplVulkanH_CreateWindowSwapChain(vk::PhysicalDevice physical_device, vk::Device device, ImGui_ImplVulkanH_Window* wd, const VkAllocationCallbacks* allocator, int w, int h, uint32_t min_image_count);
void ImGui_ImplVulkanH_CreateWindowCommandBuffers(VkPhysicalDevice physical_device, vk::Device device, ImGui_ImplVulkanH_Window* wd, uint32_t queue_family, const VkAllocationCallbacks* allocator);

//-----------------------------------------------------------------------------
// SHADERS
//-----------------------------------------------------------------------------

// glsl_shader.vert, compiled with:
// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
/*
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

void main()
{
	Out.Color = aColor;
	Out.UV = aUV;
	gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
}
*/
static uint32_t __glsl_shader_vert_spv[] =
{
	0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
	0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
	0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
	0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
	0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
	0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
	0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
	0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
	0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
	0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
	0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
	0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
	0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
	0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
	0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
	0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
	0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
	0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
	0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
	0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
	0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
	0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
	0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
	0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
	0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
	0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
	0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
	0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
	0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
	0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
	0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
	0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
	0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
	0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
	0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
	0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
	0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
	0x0000002d,0x0000002c,0x000100fd,0x00010038
};

// glsl_shader.frag, compiled with:
// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
/*
#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
	fColor = In.Color * texture(sTexture, In.UV.st);
}
*/
static uint32_t __glsl_shader_frag_spv[] =
{
	0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
	0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
	0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
	0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
	0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
	0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
	0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
	0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
	0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
	0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
	0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
	0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
	0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
	0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
	0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
	0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
	0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
	0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
	0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
	0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
	0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
	0x00010038
};

//-----------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not tested and probably dysfunctional in this backend.
static ImGui_ImplVulkan_Data* ImGui_ImplVulkan_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_ImplVulkan_Data*)ImGui::GetIO().BackendRendererUserData : NULL;
}

static uint32_t ImGui_ImplVulkan_MemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits)
{
	ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
	ImGui_ImplVulkan_InitInfo* v = &bd->VulkanInitInfo;
	VkPhysicalDeviceMemoryProperties prop;
	vkGetPhysicalDeviceMemoryProperties(v->PhysicalDevice, &prop);
	for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
		if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
			return i;
	return 0xFFFFFFFF; // Unable to find memoryType
}

static void check_vk_result(VkResult err)
{
	ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
	if (!bd)
		return;
	ImGui_ImplVulkan_InitInfo* v = &bd->VulkanInitInfo;
	if (v->CheckVkResultFn)
		v->CheckVkResultFn(err);
}

static void CreateOrResizeBuffer(vk::Buffer& buffer, VkDeviceMemory& buffer_memory, VkDeviceSize& p_buffer_size, size_t new_size, vk::BufferUsageFlagBits usage)
{
	ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
	ImGui_ImplVulkan_InitInfo* v = &bd->VulkanInitInfo;
	VkResult err;
	if (buffer)
		vkDestroyBuffer(v->Device, buffer, v->Allocator);
	if (buffer_memory != VK_NULL_HANDLE)
		vkFreeMemory(v->Device, buffer_memory, v->Allocator);

	vk::DeviceSize vertex_buffer_size_aligned = ((new_size - 1) / bd->BufferMemoryAlignment + 1) * bd->BufferMemoryAlignment;
	
	auto buffer_create_info = vk::BufferCreateInfo()
		.setSize(vertex_buffer_size_aligned)
		.setUsage(usage)
		.setSharingMode(vk::SharingMode::eExclusive);

	buffer = v->Device.createBuffer(buffer_create_info);

	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(v->Device, buffer, &req);
	bd->BufferMemoryAlignment = (bd->BufferMemoryAlignment > req.alignment) ? bd->BufferMemoryAlignment : req.alignment;
	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = req.size;
	alloc_info.memoryTypeIndex = ImGui_ImplVulkan_MemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
	err = vkAllocateMemory(v->Device, &alloc_info, v->Allocator, &buffer_memory);
	check_vk_result(err);

	err = vkBindBufferMemory(v->Device, buffer, buffer_memory, 0);
	check_vk_result(err);
	p_buffer_size = req.size;
}

static void ImGui_ImplVulkan_SetupRenderState(ImDrawData* draw_data, VkPipeline pipeline, vk::CommandBuffer command_buffer, ImGui_ImplVulkanH_FrameRenderBuffers* rb, int fb_width, int fb_height)
{
	auto bd = ImGui_ImplVulkan_GetBackendData();

	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	if (draw_data->TotalVtxCount > 0)
	{
		command_buffer.bindVertexBuffers(0, { rb->VertexBuffer }, { 0 });
		command_buffer.bindIndexBuffer(rb->IndexBuffer, 0, sizeof(ImDrawIdx) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);
	}

	auto viewport = vk::Viewport()
		.setX(0)
		.setY(0)
		.setWidth((float)fb_width)
		.setHeight((float)fb_height)
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);

	command_buffer.setViewport(0, { viewport });

	float scale[2];
	scale[0] = 2.0f / draw_data->DisplaySize.x;
	scale[1] = 2.0f / draw_data->DisplaySize.y;
	command_buffer.pushConstants(bd->PipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(float) * 0, sizeof(float) * 2, scale);
	
	float translate[2];
	translate[0] = -1.0f - draw_data->DisplayPos.x * scale[0];
	translate[1] = -1.0f - draw_data->DisplayPos.y * scale[1];
	vkCmdPushConstants(command_buffer, bd->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);
	
}

// Render function
void ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, VkCommandBuffer command_buffer, VkPipeline pipeline)
{
	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
	int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
	if (fb_width <= 0 || fb_height <= 0)
		return;

	ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
	ImGui_ImplVulkan_InitInfo* v = &bd->VulkanInitInfo;
	if (pipeline == VK_NULL_HANDLE)
		pipeline = bd->Pipeline;

	// Allocate array to store enough vertex/index buffers
	ImGui_ImplVulkanH_WindowRenderBuffers* wrb = &bd->MainWindowRenderBuffers;
	if (wrb->FrameRenderBuffers == NULL)
	{
		wrb->Index = 0;
		wrb->Count = v->ImageCount;
		wrb->FrameRenderBuffers = (ImGui_ImplVulkanH_FrameRenderBuffers*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_FrameRenderBuffers) * wrb->Count);
		memset(wrb->FrameRenderBuffers, 0, sizeof(ImGui_ImplVulkanH_FrameRenderBuffers) * wrb->Count);
	}
	IM_ASSERT(wrb->Count == v->ImageCount);
	wrb->Index = (wrb->Index + 1) % wrb->Count;
	ImGui_ImplVulkanH_FrameRenderBuffers* rb = &wrb->FrameRenderBuffers[wrb->Index];

	if (draw_data->TotalVtxCount > 0)
	{
		// Create or resize the vertex/index buffers
		size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
		size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
		if (!rb->VertexBuffer || rb->VertexBufferSize < vertex_size)
			CreateOrResizeBuffer(rb->VertexBuffer, rb->VertexBufferMemory, rb->VertexBufferSize, vertex_size, vk::BufferUsageFlagBits::eVertexBuffer);
		if (!rb->IndexBuffer || rb->IndexBufferSize < index_size)
			CreateOrResizeBuffer(rb->IndexBuffer, rb->IndexBufferMemory, rb->IndexBufferSize, index_size, vk::BufferUsageFlagBits::eIndexBuffer);

		// Upload vertex/index data into a single contiguous GPU buffer
		ImDrawVert* vtx_dst = NULL;
		ImDrawIdx* idx_dst = NULL;
		VkResult err = vkMapMemory(v->Device, rb->VertexBufferMemory, 0, rb->VertexBufferSize, 0, (void**)(&vtx_dst));
		check_vk_result(err);
		err = vkMapMemory(v->Device, rb->IndexBufferMemory, 0, rb->IndexBufferSize, 0, (void**)(&idx_dst));
		check_vk_result(err);
		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[n];
			memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtx_dst += cmd_list->VtxBuffer.Size;
			idx_dst += cmd_list->IdxBuffer.Size;
		}
		VkMappedMemoryRange range[2] = {};
		range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[0].memory = rb->VertexBufferMemory;
		range[0].size = VK_WHOLE_SIZE;
		range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[1].memory = rb->IndexBufferMemory;
		range[1].size = VK_WHOLE_SIZE;
		err = vkFlushMappedMemoryRanges(v->Device, 2, range);
		check_vk_result(err);
		vkUnmapMemory(v->Device, rb->VertexBufferMemory);
		vkUnmapMemory(v->Device, rb->IndexBufferMemory);
	}

	// Setup desired Vulkan state
	ImGui_ImplVulkan_SetupRenderState(draw_data, pipeline, command_buffer, rb, fb_width, fb_height);

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	int global_vtx_offset = 0;
	int global_idx_offset = 0;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback != NULL)
			{
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					ImGui_ImplVulkan_SetupRenderState(draw_data, pipeline, command_buffer, rb, fb_width, fb_height);
				else
					pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
				ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

				// Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
				if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
				if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
				if (clip_max.x > fb_width) { clip_max.x = (float)fb_width; }
				if (clip_max.y > fb_height) { clip_max.y = (float)fb_height; }
				if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
					continue;

				// Apply scissor/clipping rectangle
				VkRect2D scissor;
				scissor.offset.x = (int32_t)(clip_min.x);
				scissor.offset.y = (int32_t)(clip_min.y);
				scissor.extent.width = (uint32_t)(clip_max.x - clip_min.x);
				scissor.extent.height = (uint32_t)(clip_max.y - clip_min.y);
				vkCmdSetScissor(command_buffer, 0, 1, &scissor);

				// Bind DescriptorSet with font or user texture
				VkDescriptorSet desc_set[1] = { (VkDescriptorSet)pcmd->TextureId };
				if (sizeof(ImTextureID) < sizeof(ImU64))
				{
					// We don't support texture switches if ImTextureID hasn't been redefined to be 64-bit. Do a flaky check that other textures haven't been used.
					IM_ASSERT(pcmd->TextureId == (ImTextureID)bd->FontDescriptorSet);
					desc_set[0] = bd->FontDescriptorSet;
				}
				vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bd->PipelineLayout, 0, 1, desc_set, 0, NULL);

				// Draw
				vkCmdDrawIndexed(command_buffer, pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
			}
		}
		global_idx_offset += cmd_list->IdxBuffer.Size;
		global_vtx_offset += cmd_list->VtxBuffer.Size;
	}

	// Note: at this point both vkCmdSetViewport() and vkCmdSetScissor() have been called.
	// Our last values will leak into user/application rendering IF:
	// - Your app uses a pipeline with VK_DYNAMIC_STATE_VIEWPORT or VK_DYNAMIC_STATE_SCISSOR dynamic state
	// - And you forgot to call vkCmdSetViewport() and vkCmdSetScissor() yourself to explicitely set that state.
	// If you use VK_DYNAMIC_STATE_VIEWPORT or VK_DYNAMIC_STATE_SCISSOR you are responsible for setting the values before rendering.
	// In theory we should aim to backup/restore those values but I am not sure this is possible.
	// We perform a call to vkCmdSetScissor() to set back a full viewport which is likely to fix things for 99% users but technically this is not perfect. (See github #4644)
	VkRect2D scissor = { { 0, 0 }, { (uint32_t)fb_width, (uint32_t)fb_height } };
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

bool ImGui_ImplVulkan_CreateFontsTexture(VkCommandBuffer command_buffer)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
	ImGui_ImplVulkan_InitInfo* v = &bd->VulkanInitInfo;

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	size_t upload_size = width * height * 4 * sizeof(char);

	VkResult err;

	// Create the Image:
	{
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.extent.width = width;
		info.extent.height = height;
		info.extent.depth = 1;
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		err = vkCreateImage(v->Device, &info, v->Allocator, &bd->FontImage);
		check_vk_result(err);
		VkMemoryRequirements req;
		vkGetImageMemoryRequirements(v->Device, bd->FontImage, &req);
		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = req.size;
		alloc_info.memoryTypeIndex = ImGui_ImplVulkan_MemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);
		err = vkAllocateMemory(v->Device, &alloc_info, v->Allocator, &bd->FontMemory);
		check_vk_result(err);
		err = vkBindImageMemory(v->Device, bd->FontImage, bd->FontMemory, 0);
		check_vk_result(err);
	}

	// Create the Image View:
	{
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = bd->FontImage;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.layerCount = 1;
		err = vkCreateImageView(v->Device, &info, v->Allocator, &bd->FontView);
		check_vk_result(err);
	}

	bd->FontDescriptorSet = ImGui_ImplVulkan_AddTexture(bd->FontSampler, bd->FontView, vk::ImageLayout::eShaderReadOnlyOptimal);

	// Create the Upload Buffer:
	{
		VkBufferCreateInfo buffer_info = {};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = upload_size;
		buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		err = vkCreateBuffer(v->Device, &buffer_info, v->Allocator, &bd->UploadBuffer);
		check_vk_result(err);
		VkMemoryRequirements req;
		vkGetBufferMemoryRequirements(v->Device, bd->UploadBuffer, &req);
		bd->BufferMemoryAlignment = (bd->BufferMemoryAlignment > req.alignment) ? bd->BufferMemoryAlignment : req.alignment;
		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = req.size;
		alloc_info.memoryTypeIndex = ImGui_ImplVulkan_MemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
		err = vkAllocateMemory(v->Device, &alloc_info, v->Allocator, &bd->UploadBufferMemory);
		check_vk_result(err);
		err = vkBindBufferMemory(v->Device, bd->UploadBuffer, bd->UploadBufferMemory, 0);
		check_vk_result(err);
	}

	// Upload to Buffer:
	{
		char* map = NULL;
		err = vkMapMemory(v->Device, bd->UploadBufferMemory, 0, upload_size, 0, (void**)(&map));
		check_vk_result(err);
		memcpy(map, pixels, upload_size);
		VkMappedMemoryRange range[1] = {};
		range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[0].memory = bd->UploadBufferMemory;
		range[0].size = upload_size;
		err = vkFlushMappedMemoryRanges(v->Device, 1, range);
		check_vk_result(err);
		vkUnmapMemory(v->Device, bd->UploadBufferMemory);
	}

	// Copy to Image:
	{
		VkImageMemoryBarrier copy_barrier[1] = {};
		copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copy_barrier[0].image = bd->FontImage;
		copy_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy_barrier[0].subresourceRange.levelCount = 1;
		copy_barrier[0].subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, copy_barrier);

		VkBufferImageCopy region = {};
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.layerCount = 1;
		region.imageExtent.width = width;
		region.imageExtent.height = height;
		region.imageExtent.depth = 1;
		vkCmdCopyBufferToImage(command_buffer, bd->UploadBuffer, bd->FontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		VkImageMemoryBarrier use_barrier[1] = {};
		use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		use_barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		use_barrier[0].image = bd->FontImage;
		use_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		use_barrier[0].subresourceRange.levelCount = 1;
		use_barrier[0].subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, use_barrier);
	}

	// Store our identifier
	io.Fonts->SetTexID((ImTextureID)bd->FontDescriptorSet);

	return true;
}

static void ImGui_ImplVulkan_CreateShaderModules(VkDevice device, const VkAllocationCallbacks* allocator)
{
	// Create the shader modules
	ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
	if (bd->ShaderModuleVert == VK_NULL_HANDLE)
	{
		VkShaderModuleCreateInfo vert_info = {};
		vert_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vert_info.codeSize = sizeof(__glsl_shader_vert_spv);
		vert_info.pCode = (uint32_t*)__glsl_shader_vert_spv;
		VkResult err = vkCreateShaderModule(device, &vert_info, allocator, &bd->ShaderModuleVert);
		check_vk_result(err);
	}
	if (bd->ShaderModuleFrag == VK_NULL_HANDLE)
	{
		VkShaderModuleCreateInfo frag_info = {};
		frag_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		frag_info.codeSize = sizeof(__glsl_shader_frag_spv);
		frag_info.pCode = (uint32_t*)__glsl_shader_frag_spv;
		VkResult err = vkCreateShaderModule(device, &frag_info, allocator, &bd->ShaderModuleFrag);
		check_vk_result(err);
	}
}

static void ImGui_ImplVulkan_CreateFontSampler(vk::Device device, const VkAllocationCallbacks* allocator)
{
	auto bd = ImGui_ImplVulkan_GetBackendData();
	
	if (bd->FontSampler)
		return;

	// Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling.

	auto sampler_create_info = vk::SamplerCreateInfo()
		.setMagFilter(vk::Filter::eLinear)
		.setMinFilter(vk::Filter::eLinear)
		.setMipmapMode(vk::SamplerMipmapMode::eLinear)
		.setAddressModeU(vk::SamplerAddressMode::eRepeat)
		.setAddressModeV(vk::SamplerAddressMode::eRepeat)
		.setAddressModeW(vk::SamplerAddressMode::eRepeat)
		.setMinLod(-1000)
		.setMaxLod(1000)
		.setMaxAnisotropy(1.0f);

	bd->FontSampler = device.createSampler(sampler_create_info);
}

static void ImGui_ImplVulkan_CreateDescriptorSetLayout(vk::Device device, const VkAllocationCallbacks* allocator)
{
	auto  bd = ImGui_ImplVulkan_GetBackendData();
	
	if (bd->DescriptorSetLayout)
		return;

	ImGui_ImplVulkan_CreateFontSampler(device, allocator);
	
	auto descriptor_set_layout_binding = vk::DescriptorSetLayoutBinding()
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		.setPImmutableSamplers(&bd->FontSampler);

	auto descriptor_set_layout_create_info = vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount(1)
		.setPBindings(&descriptor_set_layout_binding);

	bd->DescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info);
}

static void ImGui_ImplVulkan_CreatePipelineLayout(vk::Device device, const VkAllocationCallbacks* allocator)
{
	auto bd = ImGui_ImplVulkan_GetBackendData();

	if (bd->PipelineLayout)
		return;

	// Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
	
	ImGui_ImplVulkan_CreateDescriptorSetLayout(device, allocator);
	
	// TODO: this part of code are copypasted

	auto push_constant_range = vk::PushConstantRange()
		.setStageFlags(vk::ShaderStageFlagBits::eVertex)
		.setOffset(0)
		.setSize(sizeof(float) * 4);

	auto pipeline_layout_create_info = vk::PipelineLayoutCreateInfo()
		.setSetLayoutCount(1)
		.setPSetLayouts(&bd->DescriptorSetLayout)
		.setPushConstantRangeCount(1)
		.setPPushConstantRanges(&push_constant_range);

	bd->PipelineLayout = device.createPipelineLayout(pipeline_layout_create_info);
}

static void ImGui_ImplVulkan_CreatePipeline(VkDevice device, const VkAllocationCallbacks* allocator, VkPipelineCache pipelineCache, VkRenderPass renderPass, VkSampleCountFlagBits MSAASamples, VkPipeline* pipeline, uint32_t subpass)
{
	ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
	ImGui_ImplVulkan_CreateShaderModules(device, allocator);

	VkPipelineShaderStageCreateInfo stage[2] = {};
	stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stage[0].module = bd->ShaderModuleVert;
	stage[0].pName = "main";
	stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stage[1].module = bd->ShaderModuleFrag;
	stage[1].pName = "main";

	VkVertexInputBindingDescription binding_desc[1] = {};
	binding_desc[0].stride = sizeof(ImDrawVert);
	binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attribute_desc[3] = {};
	attribute_desc[0].location = 0;
	attribute_desc[0].binding = binding_desc[0].binding;
	attribute_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
	attribute_desc[0].offset = IM_OFFSETOF(ImDrawVert, pos);
	attribute_desc[1].location = 1;
	attribute_desc[1].binding = binding_desc[0].binding;
	attribute_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
	attribute_desc[1].offset = IM_OFFSETOF(ImDrawVert, uv);
	attribute_desc[2].location = 2;
	attribute_desc[2].binding = binding_desc[0].binding;
	attribute_desc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	attribute_desc[2].offset = IM_OFFSETOF(ImDrawVert, col);

	VkPipelineVertexInputStateCreateInfo vertex_info = {};
	vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_info.vertexBindingDescriptionCount = 1;
	vertex_info.pVertexBindingDescriptions = binding_desc;
	vertex_info.vertexAttributeDescriptionCount = 3;
	vertex_info.pVertexAttributeDescriptions = attribute_desc;

	VkPipelineInputAssemblyStateCreateInfo ia_info = {};
	ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewport_info = {};
	viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_info.viewportCount = 1;
	viewport_info.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo raster_info = {};
	raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster_info.polygonMode = VK_POLYGON_MODE_FILL;
	raster_info.cullMode = VK_CULL_MODE_NONE;
	raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	raster_info.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo ms_info = {};
	ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms_info.rasterizationSamples = (MSAASamples != 0) ? MSAASamples : VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState color_attachment[1] = {};
	color_attachment[0].blendEnable = VK_TRUE;
	color_attachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_attachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_attachment[0].colorBlendOp = VK_BLEND_OP_ADD;
	color_attachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_attachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_attachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
	color_attachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineDepthStencilStateCreateInfo depth_info = {};
	depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	VkPipelineColorBlendStateCreateInfo blend_info = {};
	blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend_info.attachmentCount = 1;
	blend_info.pAttachments = color_attachment;

	VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamic_state = {};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = (uint32_t)IM_ARRAYSIZE(dynamic_states);
	dynamic_state.pDynamicStates = dynamic_states;

	ImGui_ImplVulkan_CreatePipelineLayout(device, allocator);

	VkGraphicsPipelineCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.flags = bd->PipelineCreateFlags;
	info.stageCount = 2;
	info.pStages = stage;
	info.pVertexInputState = &vertex_info;
	info.pInputAssemblyState = &ia_info;
	info.pViewportState = &viewport_info;
	info.pRasterizationState = &raster_info;
	info.pMultisampleState = &ms_info;
	info.pDepthStencilState = &depth_info;
	info.pColorBlendState = &blend_info;
	info.pDynamicState = &dynamic_state;
	info.layout = bd->PipelineLayout;
	info.renderPass = renderPass;
	info.subpass = subpass;
	VkResult err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &info, allocator, pipeline);
	check_vk_result(err);
}

bool ImGui_ImplVulkan_CreateDeviceObjects()
{
	auto bd = ImGui_ImplVulkan_GetBackendData();
	auto v = &bd->VulkanInitInfo;
	
	if (!bd->FontSampler)
	{
		auto sampler_create_info = vk::SamplerCreateInfo()
			.setMagFilter(vk::Filter::eLinear)
			.setMinFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			.setAddressModeV(vk::SamplerAddressMode::eRepeat)
			.setAddressModeW(vk::SamplerAddressMode::eRepeat)
			.setMinLod(-1000)
			.setMaxLod(1000)
			.setMaxAnisotropy(1.0f);

		bd->FontSampler = v->Device.createSampler(sampler_create_info);
	}

	if (!bd->DescriptorSetLayout)
	{
		// TODO: this part of code are copy pasted

		auto descriptor_set_layout_binding = vk::DescriptorSetLayoutBinding()
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setDescriptorCount(1)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment)
			.setPImmutableSamplers(&bd->FontSampler);

		auto descriptor_set_layout_create_info = vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(1)
			.setPBindings(&descriptor_set_layout_binding);

		bd->DescriptorSetLayout = v->Device.createDescriptorSetLayout(descriptor_set_layout_create_info);
	}

	if (!bd->PipelineLayout)
	{
		// Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
		auto push_constant_range = vk::PushConstantRange()
			.setStageFlags(vk::ShaderStageFlagBits::eVertex)
			.setOffset(0)
			.setSize(sizeof(float) * 4);

		auto pipeline_layout_create_info = vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(1)
			.setPSetLayouts(&bd->DescriptorSetLayout)
			.setPushConstantRangeCount(1)
			.setPPushConstantRanges(&push_constant_range);

		bd->PipelineLayout = v->Device.createPipelineLayout(pipeline_layout_create_info);
	}

	ImGui_ImplVulkan_CreatePipeline(v->Device, v->Allocator, v->PipelineCache, bd->RenderPass, v->MSAASamples, &bd->Pipeline, bd->Subpass);

	return true;
}

void    ImGui_ImplVulkan_DestroyFontUploadObjects()
{
	ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
	ImGui_ImplVulkan_InitInfo* v = &bd->VulkanInitInfo;
	if (bd->UploadBuffer)
	{
		vkDestroyBuffer(v->Device, bd->UploadBuffer, v->Allocator);
		bd->UploadBuffer = VK_NULL_HANDLE;
	}
	if (bd->UploadBufferMemory)
	{
		vkFreeMemory(v->Device, bd->UploadBufferMemory, v->Allocator);
		bd->UploadBufferMemory = VK_NULL_HANDLE;
	}
}

void    ImGui_ImplVulkan_DestroyDeviceObjects()
{
	ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
	ImGui_ImplVulkan_InitInfo* v = &bd->VulkanInitInfo;
	ImGui_ImplVulkanH_DestroyWindowRenderBuffers(v->Device, &bd->MainWindowRenderBuffers, v->Allocator);
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	if (bd->ShaderModuleVert)     { vkDestroyShaderModule(v->Device, bd->ShaderModuleVert, v->Allocator); bd->ShaderModuleVert = VK_NULL_HANDLE; }
	if (bd->ShaderModuleFrag)     { vkDestroyShaderModule(v->Device, bd->ShaderModuleFrag, v->Allocator); bd->ShaderModuleFrag = VK_NULL_HANDLE; }
	if (bd->FontView)             { vkDestroyImageView(v->Device, bd->FontView, v->Allocator); bd->FontView = VK_NULL_HANDLE; }
	if (bd->FontImage)            { vkDestroyImage(v->Device, bd->FontImage, v->Allocator); bd->FontImage = VK_NULL_HANDLE; }
	if (bd->FontMemory)           { vkFreeMemory(v->Device, bd->FontMemory, v->Allocator); bd->FontMemory = VK_NULL_HANDLE; }
	if (bd->FontSampler)          { vkDestroySampler(v->Device, bd->FontSampler, v->Allocator); bd->FontSampler = VK_NULL_HANDLE; }
	if (bd->DescriptorSetLayout)  { vkDestroyDescriptorSetLayout(v->Device, bd->DescriptorSetLayout, v->Allocator); bd->DescriptorSetLayout = VK_NULL_HANDLE; }
	if (bd->PipelineLayout)       { vkDestroyPipelineLayout(v->Device, bd->PipelineLayout, v->Allocator); bd->PipelineLayout = VK_NULL_HANDLE; }
	if (bd->Pipeline)             { vkDestroyPipeline(v->Device, bd->Pipeline, v->Allocator); bd->Pipeline = VK_NULL_HANDLE; }
}

bool    ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo* info, VkRenderPass render_pass)
{
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

	// Setup backend capabilities flags
	ImGui_ImplVulkan_Data* bd = IM_NEW(ImGui_ImplVulkan_Data)();
	io.BackendRendererUserData = (void*)bd;
	io.BackendRendererName = "imgui_impl_vulkan";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

	IM_ASSERT(info->PhysicalDevice != VK_NULL_HANDLE);
	IM_ASSERT(info->Device);
	IM_ASSERT(info->Queue != VK_NULL_HANDLE);
	IM_ASSERT(info->DescriptorPool != VK_NULL_HANDLE);
	IM_ASSERT(info->MinImageCount >= 2);
	IM_ASSERT(info->ImageCount >= info->MinImageCount);
	IM_ASSERT(render_pass != VK_NULL_HANDLE);

	bd->VulkanInitInfo = *info;
	bd->RenderPass = render_pass;
	bd->Subpass = info->Subpass;

	ImGui_ImplVulkan_CreateDeviceObjects();

	return true;
}

void ImGui_ImplVulkan_Shutdown()
{
	ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
	IM_ASSERT(bd != NULL && "No renderer backend to shutdown, or already shutdown?");
	ImGuiIO& io = ImGui::GetIO();

	ImGui_ImplVulkan_DestroyDeviceObjects();
	io.BackendRendererName = NULL;
	io.BackendRendererUserData = NULL;
	IM_DELETE(bd);
}

void ImGui_ImplVulkan_NewFrame()
{
	ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
	IM_ASSERT(bd != NULL && "Did you call ImGui_ImplVulkan_Init()?");
	IM_UNUSED(bd);
}

void ImGui_ImplVulkan_SetMinImageCount(uint32_t min_image_count)
{
	ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
	IM_ASSERT(min_image_count >= 2);
	if (bd->VulkanInitInfo.MinImageCount == min_image_count)
		return;

	ImGui_ImplVulkan_InitInfo* v = &bd->VulkanInitInfo;
	VkResult err = vkDeviceWaitIdle(v->Device);
	check_vk_result(err);
	ImGui_ImplVulkanH_DestroyWindowRenderBuffers(v->Device, &bd->MainWindowRenderBuffers, v->Allocator);
	bd->VulkanInitInfo.MinImageCount = min_image_count;
}

// Register a texture
// FIXME: This is experimental in the sense that we are unsure how to best design/tackle this problem, please post to https://github.com/ocornut/imgui/pull/914 if you have suggestions.
VkDescriptorSet ImGui_ImplVulkan_AddTexture(vk::Sampler sampler, vk::ImageView image_view, vk::ImageLayout image_layout)
{
	auto bd = ImGui_ImplVulkan_GetBackendData();
	auto v = &bd->VulkanInitInfo;

	auto descriptor_set_allocate_info = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(v->DescriptorPool)
		.setDescriptorSetCount(1)
		.setPSetLayouts(&bd->DescriptorSetLayout);

	auto descriptor_set = v->Device.allocateDescriptorSets(descriptor_set_allocate_info).at(0);

	auto descriptor_image_info = vk::DescriptorImageInfo()
		.setSampler(sampler)
		.setImageView(image_view)
		.setImageLayout(image_layout);

	auto write_descriptor_set = vk::WriteDescriptorSet()
		.setDstSet(descriptor_set)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setPImageInfo(&descriptor_image_info);

	v->Device.updateDescriptorSets({ write_descriptor_set }, {});
	
	return descriptor_set;
}

//-------------------------------------------------------------------------
// Internal / Miscellaneous Vulkan Helpers
// (Used by example's main.cpp. Used by multi-viewport features. PROBABLY NOT used by your own app.)
//-------------------------------------------------------------------------
// You probably do NOT need to use or care about those functions.
// Those functions only exist because:
//   1) they facilitate the readability and maintenance of the multiple main.cpp examples files.
//   2) the upcoming multi-viewport feature will need them internally.
// Generally we avoid exposing any kind of superfluous high-level helpers in the backends,
// but it is too much code to duplicate everywhere so we exceptionally expose them.
//
// Your engine/app will likely _already_ have code to setup all that stuff (swap chain, render pass, frame buffers, etc.).
// You may read this code to learn about Vulkan, but it is recommended you use you own custom tailored code to do equivalent work.
// (The ImGui_ImplVulkanH_XXX functions do not interact with any of the state used by the regular ImGui_ImplVulkan_XXX functions)
//-------------------------------------------------------------------------

VkSurfaceFormatKHR ImGui_ImplVulkanH_SelectSurfaceFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const VkFormat* request_formats, int request_formats_count, VkColorSpaceKHR request_color_space)
{
	IM_ASSERT(request_formats != NULL);
	IM_ASSERT(request_formats_count > 0);

	// Per Spec Format and View Format are expected to be the same unless VK_IMAGE_CREATE_MUTABLE_BIT was set at image creation
	// Assuming that the default behavior is without setting this bit, there is no need for separate Swapchain image and image view format
	// Additionally several new color spaces were introduced with Vulkan Spec v1.0.40,
	// hence we must make sure that a format with the mostly available color space, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, is found and used.
	uint32_t avail_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &avail_count, NULL);
	ImVector<VkSurfaceFormatKHR> avail_format;
	avail_format.resize((int)avail_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &avail_count, avail_format.Data);

	// First check if only one format, VK_FORMAT_UNDEFINED, is available, which would imply that any format is available
	if (avail_count == 1)
	{
		if (avail_format[0].format == VK_FORMAT_UNDEFINED)
		{
			VkSurfaceFormatKHR ret;
			ret.format = request_formats[0];
			ret.colorSpace = request_color_space;
			return ret;
		}
		else
		{
			// No point in searching another format
			return avail_format[0];
		}
	}
	else
	{
		// Request several formats, the first found will be used
		for (int request_i = 0; request_i < request_formats_count; request_i++)
			for (uint32_t avail_i = 0; avail_i < avail_count; avail_i++)
				if (avail_format[avail_i].format == request_formats[request_i] && avail_format[avail_i].colorSpace == request_color_space)
					return avail_format[avail_i];

		// If none of the requested image formats could be found, use the first available
		return avail_format[0];
	}
}

vk::PresentModeKHR ImGui_ImplVulkanH_SelectPresentMode(vk::PhysicalDevice physical_device, VkSurfaceKHR surface, const std::vector<vk::PresentModeKHR>& request_modes)
{
	auto avail_modes = physical_device.getSurfacePresentModesKHR(surface);

	for (int request_i = 0; request_i < (int)request_modes.size(); request_i++)
		for (uint32_t avail_i = 0; avail_i < avail_modes.size(); avail_i++)
			if (request_modes[request_i] == avail_modes[avail_i])
				return request_modes[request_i];

	return vk::PresentModeKHR::eFifo;
}

void ImGui_ImplVulkanH_CreateWindowCommandBuffers(VkPhysicalDevice physical_device, vk::Device device, ImGui_ImplVulkanH_Window* wd, uint32_t queue_family, const VkAllocationCallbacks* allocator)
{
	for (uint32_t i = 0; i < wd->ImageCount; i++)
	{
		auto fd = &wd->Frames[i];
		auto fsd = &wd->FrameSemaphores[i];
			
		auto command_pool_create_info = vk::CommandPoolCreateInfo()
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.setQueueFamilyIndex(queue_family);                

		fd->CommandPool = device.createCommandPool(command_pool_create_info);
		
		auto command_buffer_allocate_info = vk::CommandBufferAllocateInfo()
			.setCommandPool(fd->CommandPool)
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandBufferCount(1);

		fd->CommandBuffer = device.allocateCommandBuffers(command_buffer_allocate_info).at(0);
		
		auto fence_create_info = vk::FenceCreateInfo()
			.setFlags(vk::FenceCreateFlagBits::eSignaled);

		fd->Fence = device.createFence(fence_create_info);
		
		auto semaphore_create_info = vk::SemaphoreCreateInfo();

		fsd->ImageAcquiredSemaphore = device.createSemaphore(semaphore_create_info);
		fsd->RenderCompleteSemaphore = device.createSemaphore(semaphore_create_info);
	}
}

int ImGui_ImplVulkanH_GetMinImageCountFromPresentMode(vk::PresentModeKHR present_mode)
{
	if (present_mode == vk::PresentModeKHR::eMailbox)
		return 3;
	if (present_mode == vk::PresentModeKHR::eFifo || present_mode == vk::PresentModeKHR::eFifoRelaxed)
		return 2;
	if (present_mode == vk::PresentModeKHR::eImmediate)
		return 1;
	IM_ASSERT(0);
	return 1;
}

// Also destroy old swap chain and in-flight frames data, if any.
void ImGui_ImplVulkanH_CreateWindowSwapChain(vk::PhysicalDevice physical_device, vk::Device device, ImGui_ImplVulkanH_Window* wd, const VkAllocationCallbacks* allocator, int w, int h, uint32_t min_image_count)
{
	auto old_swapchain = wd->Swapchain;
	wd->Swapchain = VK_NULL_HANDLE;

	device.waitIdle();

	// We don't use ImGui_ImplVulkanH_DestroyWindow() because we want to preserve the old swapchain to create the new one.
	// Destroy old Framebuffer
	for (uint32_t i = 0; i < wd->ImageCount; i++)
	{
		ImGui_ImplVulkanH_DestroyFrame(device, &wd->Frames[i], allocator);
		ImGui_ImplVulkanH_DestroyFrameSemaphores(device, &wd->FrameSemaphores[i], allocator);
	}
	IM_FREE(wd->Frames);
	IM_FREE(wd->FrameSemaphores);
	wd->Frames = NULL;
	wd->FrameSemaphores = NULL;
	wd->ImageCount = 0;
	if (wd->RenderPass)
		vkDestroyRenderPass(device, wd->RenderPass, allocator);
	if (wd->Pipeline)
		vkDestroyPipeline(device, wd->Pipeline, allocator);

	// If min image count was not specified, request different count of images dependent on selected present mode
	if (min_image_count == 0)
		min_image_count = ImGui_ImplVulkanH_GetMinImageCountFromPresentMode(wd->PresentMode);

	// Create Swapchain
	{
		auto swapchain_create_info = vk::SwapchainCreateInfoKHR()
			.setSurface(wd->Surface)
			.setMinImageCount(min_image_count)
			.setImageFormat(wd->SurfaceFormat.format)
			.setImageColorSpace(wd->SurfaceFormat.colorSpace)
			.setImageArrayLayers(1)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
			.setImageSharingMode(vk::SharingMode::eExclusive)
			.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
			.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
			.setPresentMode(wd->PresentMode)
			.setClipped(true)
			.setOldSwapchain(old_swapchain);

		auto capabilities = physical_device.getSurfaceCapabilitiesKHR(wd->Surface);

		if (swapchain_create_info.minImageCount < capabilities.minImageCount)
			swapchain_create_info.minImageCount = capabilities.minImageCount;
		else if (capabilities.maxImageCount != 0 && swapchain_create_info.minImageCount > capabilities.maxImageCount)
			swapchain_create_info.minImageCount = capabilities.maxImageCount;

		if (capabilities.currentExtent.width == 0xffffffff)
		{
			swapchain_create_info.imageExtent.width = wd->Width = w;
			swapchain_create_info.imageExtent.height = wd->Height = h;
		}
		else
		{
			swapchain_create_info.imageExtent.width = wd->Width = capabilities.currentExtent.width;
			swapchain_create_info.imageExtent.height = wd->Height = capabilities.currentExtent.height;
		}
		
		wd->Swapchain = device.createSwapchainKHR(swapchain_create_info);
		
		auto backbuffers = device.getSwapchainImagesKHR(wd->Swapchain);
		wd->ImageCount = (uint32_t)backbuffers.size();
		
		IM_ASSERT(wd->Frames == NULL);
		wd->Frames = (ImGui_ImplVulkanH_Frame*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_Frame) * wd->ImageCount);
		wd->FrameSemaphores = (ImGui_ImplVulkanH_FrameSemaphores*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_FrameSemaphores) * wd->ImageCount);
		memset(wd->Frames, 0, sizeof(wd->Frames[0]) * wd->ImageCount);
		memset(wd->FrameSemaphores, 0, sizeof(wd->FrameSemaphores[0]) * wd->ImageCount);
		
		for (uint32_t i = 0; i < wd->ImageCount; i++)
			wd->Frames[i].Backbuffer = backbuffers[i];
	}
	if (old_swapchain)
		device.destroySwapchainKHR(old_swapchain);

	auto attachment_description = vk::AttachmentDescription()
		.setFormat(wd->SurfaceFormat.format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(wd->ClearEnable ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
		
	auto attachment_reference = vk::AttachmentReference()
		.setAttachment(0)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	auto subpass_description = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(1)
		.setPColorAttachments(&attachment_reference);

	auto subpass_dependency = vk::SubpassDependency()
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlagBits::eNone)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

	auto render_pass_create_info = vk::RenderPassCreateInfo()
		.setAttachmentCount(1)
		.setPAttachments(&attachment_description)
		.setSubpassCount(1)
		.setPSubpasses(&subpass_description)
		.setDependencyCount(1)
		.setPDependencies(&subpass_dependency);

	wd->RenderPass = device.createRenderPass(render_pass_create_info);
		
	for (uint32_t i = 0; i < wd->ImageCount; i++)
	{
		auto fd = &wd->Frames[i];

		auto image_view_component_mapping = vk::ComponentMapping()
			.setR(vk::ComponentSwizzle::eR)
			.setG(vk::ComponentSwizzle::eG)
			.setB(vk::ComponentSwizzle::eB)
			.setA(vk::ComponentSwizzle::eA);

		auto image_view_subresource_range = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);

		auto image_view_create_info = vk::ImageViewCreateInfo()
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(wd->SurfaceFormat.format)
			.setComponents(image_view_component_mapping)
			.setSubresourceRange(image_view_subresource_range)
			.setImage(fd->Backbuffer);

		fd->BackbufferView = device.createImageView(image_view_create_info);
	}
	
	for (uint32_t i = 0; i < wd->ImageCount; i++)
	{
		auto fd = &wd->Frames[i];

		auto framebuffer_create_info = vk::FramebufferCreateInfo()
			.setRenderPass(wd->RenderPass)
			.setAttachmentCount(1)
			.setPAttachments(&fd->BackbufferView)
			.setWidth(wd->Width)
			.setHeight(wd->Height)
			.setLayers(1);

		fd->Framebuffer = device.createFramebuffer(framebuffer_create_info);
	}
}

// Create or resize window
void ImGui_ImplVulkanH_CreateOrResizeWindow(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, ImGui_ImplVulkanH_Window* wd, uint32_t queue_family, const VkAllocationCallbacks* allocator, int width, int height, uint32_t min_image_count)
{
	(void)instance;
	ImGui_ImplVulkanH_CreateWindowSwapChain(physical_device, device, wd, allocator, width, height, min_image_count);
	ImGui_ImplVulkanH_CreateWindowCommandBuffers(physical_device, device, wd, queue_family, allocator);
}

void ImGui_ImplVulkanH_DestroyWindow(vk::Instance instance, vk::Device device, ImGui_ImplVulkanH_Window* wd, const VkAllocationCallbacks* allocator)
{
	device.waitIdle(); // FIXME: We could wait on the Queue if we had the queue in wd-> (otherwise VulkanH functions can't use globals)
	//vkQueueWaitIdle(bd->Queue);

	for (uint32_t i = 0; i < wd->ImageCount; i++)
	{
		ImGui_ImplVulkanH_DestroyFrame(device, &wd->Frames[i], allocator);
		ImGui_ImplVulkanH_DestroyFrameSemaphores(device, &wd->FrameSemaphores[i], allocator);
	}
	IM_FREE(wd->Frames);
	IM_FREE(wd->FrameSemaphores);
	wd->Frames = NULL;
	wd->FrameSemaphores = NULL;
	device.destroy(wd->Pipeline);
	device.destroy(wd->RenderPass);
	device.destroy(wd->Swapchain);
	instance.destroy(wd->Surface);

	*wd = ImGui_ImplVulkanH_Window();
}

void ImGui_ImplVulkanH_DestroyFrame(vk::Device device, ImGui_ImplVulkanH_Frame* fd, const VkAllocationCallbacks* allocator)
{
	device.destroy(fd->Fence);
	device.freeCommandBuffers(fd->CommandPool, { fd->CommandBuffer });
	device.destroy(fd->CommandPool);
	fd->Fence = VK_NULL_HANDLE;
	fd->CommandBuffer = VK_NULL_HANDLE;
	fd->CommandPool = VK_NULL_HANDLE;
	
	device.destroy(fd->BackbufferView);
	device.destroy(fd->Framebuffer);
}

void ImGui_ImplVulkanH_DestroyFrameSemaphores(vk::Device device, ImGui_ImplVulkanH_FrameSemaphores* fsd, const VkAllocationCallbacks* allocator)
{
	device.destroy(fsd->ImageAcquiredSemaphore);
	device.destroy(fsd->RenderCompleteSemaphore);
	fsd->ImageAcquiredSemaphore = fsd->RenderCompleteSemaphore = VK_NULL_HANDLE;
}

void ImGui_ImplVulkanH_DestroyFrameRenderBuffers(vk::Device device, ImGui_ImplVulkanH_FrameRenderBuffers* buffers, const VkAllocationCallbacks* allocator)
{
	if (buffers->VertexBuffer) 
	{ 
		device.destroy(buffers->VertexBuffer); 
		buffers->VertexBuffer = VK_NULL_HANDLE; 
	}

	if (buffers->VertexBufferMemory) 
	{ 
		device.free(buffers->VertexBufferMemory); 
		buffers->VertexBufferMemory = VK_NULL_HANDLE; 
	}

	if (buffers->IndexBuffer) 
	{ 
		device.destroy(buffers->IndexBuffer); 
		buffers->IndexBuffer = VK_NULL_HANDLE; 
	}

	if (buffers->IndexBufferMemory) 
	{ 
		device.free(buffers->IndexBufferMemory); 
		buffers->IndexBufferMemory = VK_NULL_HANDLE; 
	}

	buffers->VertexBufferSize = 0;
	buffers->IndexBufferSize = 0;
}

void ImGui_ImplVulkanH_DestroyWindowRenderBuffers(VkDevice device, ImGui_ImplVulkanH_WindowRenderBuffers* buffers, const VkAllocationCallbacks* allocator)
{
	for (uint32_t n = 0; n < buffers->Count; n++)
		ImGui_ImplVulkanH_DestroyFrameRenderBuffers(device, &buffers->FrameRenderBuffers[n], allocator);
	IM_FREE(buffers->FrameRenderBuffers);
	buffers->FrameRenderBuffers = NULL;
	buffers->Index = 0;
	buffers->Count = 0;
}
