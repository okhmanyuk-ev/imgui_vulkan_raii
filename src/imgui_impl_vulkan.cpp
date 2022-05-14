#include "imgui_impl_vulkan.h"
#include <stdio.h>

struct ImGui_ImplVulkanH_FrameRenderBuffers
{
	vk::raii::DeviceMemory VertexBufferMemory = nullptr;
	vk::raii::DeviceMemory IndexBufferMemory = nullptr;
	vk::DeviceSize VertexBufferSize = 0;
	vk::DeviceSize IndexBufferSize = 0;
	vk::raii::Buffer VertexBuffer = nullptr;
	vk::raii::Buffer IndexBuffer = nullptr;
};

struct ImGui_ImplVulkanH_WindowRenderBuffers
{
	uint32_t Index;
	std::vector<ImGui_ImplVulkanH_FrameRenderBuffers> FrameRenderBuffers;
};

struct ImGui_ImplVulkan_Data
{
	ImGui_ImplVulkan_InitInfo VulkanInitInfo;
	vk::DeviceSize BufferMemoryAlignment;
	vk::PipelineCreateFlags PipelineCreateFlags;
	vk::raii::DescriptorSetLayout DescriptorSetLayout = nullptr;
	vk::raii::PipelineLayout PipelineLayout = nullptr;
	vk::raii::Pipeline Pipeline = nullptr;
	uint32_t Subpass;
	vk::raii::ShaderModule ShaderModuleVert = nullptr;
	vk::raii::ShaderModule ShaderModuleFrag = nullptr;

	// Font data
	vk::raii::Sampler FontSampler = nullptr;
	vk::raii::DeviceMemory FontMemory = nullptr;
	vk::raii::Image FontImage = nullptr;
	vk::raii::ImageView FontView = nullptr;
	vk::raii::DescriptorSet FontDescriptorSet = nullptr;
	vk::raii::DeviceMemory UploadBufferMemory = nullptr;
	vk::raii::Buffer UploadBuffer = nullptr;

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
void ImGui_ImplVulkanH_DestroyWindowRenderBuffers(vk::Device device, ImGui_ImplVulkanH_WindowRenderBuffers& buffers);
void ImGui_ImplVulkanH_CreateWindowSwapChain(vk::PhysicalDevice physical_device, vk::raii::Device& device, ImGui_ImplVulkanH_Window& wd, int w, int h, uint32_t min_image_count);
void ImGui_ImplVulkanH_CreateWindowCommandBuffers(vk::PhysicalDevice physical_device, vk::raii::Device& device, ImGui_ImplVulkanH_Window& wd, uint32_t queue_family);

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

static uint32_t ImGui_ImplVulkan_MemoryType(vk::MemoryPropertyFlags properties, uint32_t type_bits)
{
	auto prop = g_PhysicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
		if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
			return i;
	return 0xFFFFFFFF; // Unable to find memoryType
}

static void CreateOrResizeBuffer(vk::raii::Buffer& buffer, vk::raii::DeviceMemory& buffer_memory, vk::DeviceSize& p_buffer_size, size_t new_size, vk::BufferUsageFlagBits usage)
{
	auto bd = ImGui_ImplVulkan_GetBackendData();

	vk::DeviceSize vertex_buffer_size_aligned = ((new_size - 1) / bd->BufferMemoryAlignment + 1) * bd->BufferMemoryAlignment;
	
	auto buffer_create_info = vk::BufferCreateInfo()
		.setSize(vertex_buffer_size_aligned)
		.setUsage(usage)
		.setSharingMode(vk::SharingMode::eExclusive);

	buffer = g_Device.createBuffer(buffer_create_info);

	auto req = buffer.getMemoryRequirements();

	bd->BufferMemoryAlignment = (bd->BufferMemoryAlignment > req.alignment) ? bd->BufferMemoryAlignment : req.alignment;

	auto memory_allocate_info = vk::MemoryAllocateInfo()
		.setAllocationSize(req.size)
		.setMemoryTypeIndex(ImGui_ImplVulkan_MemoryType(vk::MemoryPropertyFlagBits::eHostVisible, req.memoryTypeBits));

	buffer_memory = g_Device.allocateMemory(memory_allocate_info);

	buffer.bindMemory(*buffer_memory, 0);

	p_buffer_size = req.size;
}

static void ImGui_ImplVulkan_SetupRenderState(ImDrawData* draw_data, vk::Pipeline pipeline, vk::CommandBuffer command_buffer, ImGui_ImplVulkanH_FrameRenderBuffers* rb, int fb_width, int fb_height)
{
	auto bd = ImGui_ImplVulkan_GetBackendData();

	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	if (draw_data->TotalVtxCount > 0)
	{
		command_buffer.bindVertexBuffers(0, { *rb->VertexBuffer }, { 0 });
		command_buffer.bindIndexBuffer(*rb->IndexBuffer, 0, sizeof(ImDrawIdx) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);
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
	command_buffer.pushConstants(*bd->PipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(float) * 0, sizeof(float) * 2, scale);
	
	float translate[2];
	translate[0] = -1.0f - draw_data->DisplayPos.x * scale[0];
	translate[1] = -1.0f - draw_data->DisplayPos.y * scale[1];
	command_buffer.pushConstants(*bd->PipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(float) * 2, sizeof(float) * 2, translate);
}

void ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, vk::CommandBuffer command_buffer)
{
	int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
	int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
	
	if (fb_width <= 0 || fb_height <= 0)
		return;

	auto bd = ImGui_ImplVulkan_GetBackendData();
	auto v = &bd->VulkanInitInfo;
	
	auto pipeline = *bd->Pipeline;

	auto wrb = &bd->MainWindowRenderBuffers;
	
	if (wrb->FrameRenderBuffers.empty())
	{
		wrb->FrameRenderBuffers.resize(v->ImageCount);
	}

	wrb->Index = (wrb->Index + 1) % wrb->FrameRenderBuffers.size();
	
	auto rb = &wrb->FrameRenderBuffers[wrb->Index];

	if (draw_data->TotalVtxCount > 0)
	{
		auto vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
		auto index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

		if (rb->VertexBufferSize < vertex_size)
			CreateOrResizeBuffer(rb->VertexBuffer, rb->VertexBufferMemory, rb->VertexBufferSize, vertex_size, vk::BufferUsageFlagBits::eVertexBuffer);

		if (rb->IndexBufferSize < index_size)
			CreateOrResizeBuffer(rb->IndexBuffer, rb->IndexBufferMemory, rb->IndexBufferSize, index_size, vk::BufferUsageFlagBits::eIndexBuffer);

		auto vtx_dst = (ImDrawVert*)(*g_Device).mapMemory(*rb->VertexBufferMemory, 0, rb->VertexBufferSize);
		auto idx_dst = (ImDrawIdx*)(*g_Device).mapMemory(*rb->IndexBufferMemory, 0, rb->IndexBufferSize);

		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const auto cmd_list = draw_data->CmdLists[n];
			memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtx_dst += cmd_list->VtxBuffer.Size;
			idx_dst += cmd_list->IdxBuffer.Size;
		}

		auto vertex_memory_range = vk::MappedMemoryRange()
			.setMemory(*rb->VertexBufferMemory)
			.setSize(VK_WHOLE_SIZE);

		auto index_memory_range = vk::MappedMemoryRange()
			.setMemory(*rb->IndexBufferMemory)
			.setSize(VK_WHOLE_SIZE);
		
		g_Device.flushMappedMemoryRanges({ vertex_memory_range, index_memory_range });

		rb->VertexBufferMemory.unmapMemory();
		rb->IndexBufferMemory.unmapMemory();
	}

	ImGui_ImplVulkan_SetupRenderState(draw_data, pipeline, command_buffer, rb, fb_width, fb_height);

	auto clip_off = draw_data->DisplayPos;
	auto clip_scale = draw_data->FramebufferScale;

	int global_vtx_offset = 0;
	int global_idx_offset = 0;

	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const auto cmd_list = draw_data->CmdLists[n];
		
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const auto pcmd = &cmd_list->CmdBuffer[cmd_i];
		
			if (pcmd->UserCallback != NULL)
			{
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					ImGui_ImplVulkan_SetupRenderState(draw_data, pipeline, command_buffer, rb, fb_width, fb_height);
				else
					pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
				ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

				if (clip_min.x < 0.0f) 
					clip_min.x = 0.0f;

				if (clip_min.y < 0.0f) 
					clip_min.y = 0.0f;

				if (clip_max.x > fb_width)
					clip_max.x = (float)fb_width;

				if (clip_max.y > fb_height)
					clip_max.y = (float)fb_height;

				if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
					continue;

				auto scissor = vk::Rect2D()
					.setOffset({ (int32_t)clip_min.x, (int32_t)clip_min.y })
					.setExtent({ (uint32_t)(clip_max.x - clip_min.x), (uint32_t)(clip_max.y - clip_min.y) });

				command_buffer.setScissor(0, { scissor });

				auto desc_set = (vk::DescriptorSet)(VkDescriptorSet)pcmd->TextureId;

				if (sizeof(ImTextureID) < sizeof(ImU64))
				{
					// We don't support texture switches if ImTextureID hasn't been redefined to be 64-bit. Do a flaky check that other textures haven't been used.
					IM_ASSERT(pcmd->TextureId == (ImTextureID)*bd->FontDescriptorSet);
					desc_set = *bd->FontDescriptorSet;
				}

				command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *bd->PipelineLayout, 0, { desc_set }, {});
				command_buffer.drawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
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
	
	auto scissor = vk::Rect2D()
		.setOffset({ 0, 0 })
		.setExtent({ (uint32_t)fb_width, (uint32_t)fb_height });

	command_buffer.setScissor(0, { scissor });
}

bool ImGui_ImplVulkan_CreateFontsTexture(vk::CommandBuffer command_buffer)
{
	auto& io = ImGui::GetIO();
	auto bd = ImGui_ImplVulkan_GetBackendData();
	auto v = &bd->VulkanInitInfo;

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	size_t upload_size = width * height * 4 * sizeof(char);

	{
		auto image_create_info = vk::ImageCreateInfo()
			.setImageType(vk::ImageType::e2D)
			.setFormat(vk::Format::eR8G8B8A8Unorm)
			.setExtent({ (uint32_t)width, (uint32_t)height, 1 })
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
			.setSharingMode(vk::SharingMode::eExclusive)
			.setInitialLayout(vk::ImageLayout::eUndefined);
			
		bd->FontImage = g_Device.createImage(image_create_info);

		auto req = bd->FontImage.getMemoryRequirements();

		auto memory_allocate_info = vk::MemoryAllocateInfo()
			.setAllocationSize(req.size)
			.setMemoryTypeIndex(ImGui_ImplVulkan_MemoryType(vk::MemoryPropertyFlagBits::eDeviceLocal, req.memoryTypeBits));

		bd->FontMemory = g_Device.allocateMemory(memory_allocate_info);
		bd->FontImage.bindMemory(*bd->FontMemory, 0);
	}

	{
		auto image_subresource_range = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setLevelCount(1)
			.setLayerCount(1);

		auto image_view_create_info = vk::ImageViewCreateInfo()
			.setImage(*bd->FontImage)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(vk::Format::eR8G8B8A8Unorm)
			.setSubresourceRange(image_subresource_range);

		bd->FontView = g_Device.createImageView(image_view_create_info);
	}

	bd->FontDescriptorSet = ImGui_ImplVulkan_AddTexture(*bd->FontSampler, *bd->FontView, vk::ImageLayout::eShaderReadOnlyOptimal);

	// Create the Upload Buffer:
	{
		auto buffer_create_info = vk::BufferCreateInfo()
			.setSize(upload_size)
			.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
			.setSharingMode(vk::SharingMode::eExclusive);

		bd->UploadBuffer = g_Device.createBuffer(buffer_create_info);

		auto req = bd->UploadBuffer.getMemoryRequirements();

		bd->BufferMemoryAlignment = (bd->BufferMemoryAlignment > req.alignment) ? bd->BufferMemoryAlignment : req.alignment;

		auto memory_allocate_info = vk::MemoryAllocateInfo()
			.setAllocationSize(req.size)
			.setMemoryTypeIndex(ImGui_ImplVulkan_MemoryType(vk::MemoryPropertyFlagBits::eHostVisible, req.memoryTypeBits));

		bd->UploadBufferMemory = g_Device.allocateMemory(memory_allocate_info);
		bd->UploadBuffer.bindMemory(*bd->UploadBufferMemory, 0);
	}

	{
		auto map = bd->UploadBufferMemory.mapMemory(0, upload_size);

		memcpy(map, pixels, upload_size);
		
		auto range = vk::MappedMemoryRange()
			.setMemory(*bd->UploadBufferMemory)
			.setSize(upload_size);

		g_Device.flushMappedMemoryRanges({ range });

		bd->UploadBufferMemory.unmapMemory();
	}

	{
		auto copy_image_subresource_range = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setLevelCount(1)
			.setLayerCount(1);

		auto copy_image_memory_barrier= vk::ImageMemoryBarrier()
			.setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setOldLayout(vk::ImageLayout::eUndefined)
			.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setImage(*bd->FontImage)
			.setSubresourceRange(copy_image_subresource_range);

		command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, 
			{}, {}, {}, { copy_image_memory_barrier });

		auto image_subresource_layers = vk::ImageSubresourceLayers()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setLayerCount(1);

		auto region = vk::BufferImageCopy()
			.setImageSubresource(image_subresource_layers)
			.setImageExtent({ (uint32_t)width, (uint32_t)height, 1 });

		command_buffer.copyBufferToImage(*bd->UploadBuffer, *bd->FontImage, vk::ImageLayout::eTransferDstOptimal, { region });

		auto use_image_subresource_range = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setLevelCount(1)
			.setLayerCount(1);

		auto use_image_memory_barrier = vk::ImageMemoryBarrier()
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
			.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
			.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setImage(*bd->FontImage)
			.setSubresourceRange(use_image_subresource_range);

		command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, 
			{}, {}, {}, { use_image_memory_barrier });
	}

	io.Fonts->SetTexID((ImTextureID)*bd->FontDescriptorSet);

	return true;
}

static void ImGui_ImplVulkan_CreateShaderModules(vk::raii::Device& device)
{
	auto bd = ImGui_ImplVulkan_GetBackendData();
	
	{
		auto shader_modeul_create_info = vk::ShaderModuleCreateInfo()
			.setCodeSize(sizeof(__glsl_shader_vert_spv))
			.setPCode((uint32_t*)__glsl_shader_vert_spv);

		bd->ShaderModuleVert = device.createShaderModule(shader_modeul_create_info);
	}
	{
		auto shader_modeul_create_info = vk::ShaderModuleCreateInfo()
			.setCodeSize(sizeof(__glsl_shader_frag_spv))
			.setPCode((uint32_t*)__glsl_shader_frag_spv);

		bd->ShaderModuleFrag = device.createShaderModule(shader_modeul_create_info);
	}
}

static void ImGui_ImplVulkan_CreateFontSampler(vk::raii::Device& device)
{
	auto bd = ImGui_ImplVulkan_GetBackendData();
	
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

static void ImGui_ImplVulkan_CreateDescriptorSetLayout(vk::raii::Device& device)
{
	auto  bd = ImGui_ImplVulkan_GetBackendData();
	
	ImGui_ImplVulkan_CreateFontSampler(device);
	
	auto descriptor_set_layout_binding = vk::DescriptorSetLayoutBinding()
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		.setPImmutableSamplers(&*bd->FontSampler);

	auto descriptor_set_layout_create_info = vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount(1)
		.setPBindings(&descriptor_set_layout_binding);

	bd->DescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info);
}

static void ImGui_ImplVulkan_CreatePipelineLayout(vk::raii::Device& device)
{
	auto bd = ImGui_ImplVulkan_GetBackendData();

	// Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
	
	ImGui_ImplVulkan_CreateDescriptorSetLayout(device);
	
	// TODO: this part of code are copypasted

	auto push_constant_range = vk::PushConstantRange()
		.setStageFlags(vk::ShaderStageFlagBits::eVertex)
		.setOffset(0)
		.setSize(sizeof(float) * 4);

	auto pipeline_layout_create_info = vk::PipelineLayoutCreateInfo()
		.setSetLayoutCount(1)
		.setPSetLayouts(&*bd->DescriptorSetLayout)
		.setPushConstantRangeCount(1)
		.setPPushConstantRanges(&push_constant_range);

	bd->PipelineLayout = device.createPipelineLayout(pipeline_layout_create_info);
}

static void ImGui_ImplVulkan_CreatePipeline(vk::raii::Device& device, const vk::raii::PipelineCache& pipelineCache, vk::raii::RenderPass& renderPass, vk::raii::Pipeline& pipeline, uint32_t subpass)
{
	auto bd = ImGui_ImplVulkan_GetBackendData();
	ImGui_ImplVulkan_CreateShaderModules(device);

	auto pipeline_shader_stage_create_info = {
		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(*bd->ShaderModuleVert)
			.setPName("main"),

		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(*bd->ShaderModuleFrag)
			.setPName("main")
	};

	auto vertex_input_binding_description = vk::VertexInputBindingDescription()
		.setStride(sizeof(ImDrawVert))
		.setInputRate(vk::VertexInputRate::eVertex)
		.setBinding(0);

	auto vertex_input_attribute_descriptions = {
		vk::VertexInputAttributeDescription()
			.setLocation(0)
			.setBinding(vertex_input_binding_description.binding)
			.setFormat(vk::Format::eR32G32Sfloat)
			.setOffset(IM_OFFSETOF(ImDrawVert, pos)), // TODO: change IM_OFFSETOF -> offsetof

		vk::VertexInputAttributeDescription()
			.setLocation(1)
			.setBinding(vertex_input_binding_description.binding)
			.setFormat(vk::Format::eR32G32Sfloat)
			.setOffset(IM_OFFSETOF(ImDrawVert, uv)),

		vk::VertexInputAttributeDescription()
			.setLocation(2)
			.setBinding(vertex_input_binding_description.binding)
			.setFormat(vk::Format::eR8G8B8A8Unorm)
			.setOffset(IM_OFFSETOF(ImDrawVert, col))
	};

	auto pipeline_vertex_input_state_create_info = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptionCount(1)
		.setPVertexBindingDescriptions(&vertex_input_binding_description)
		.setVertexAttributeDescriptions(vertex_input_attribute_descriptions);

	auto pipeline_input_assembly_state_create_info = vk::PipelineInputAssemblyStateCreateInfo() 
		.setTopology(vk::PrimitiveTopology::eTriangleList); // TODO: can be dynamic state

	auto pipeline_viewport_state_create_info = vk::PipelineViewportStateCreateInfo()
		.setViewportCount(1)
		.setScissorCount(1);

	auto pipeline_rasterization_state_create_info = vk::PipelineRasterizationStateCreateInfo()
		.setPolygonMode(vk::PolygonMode::eFill)
		.setCullMode(vk::CullModeFlagBits::eNone)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setLineWidth(1.0f);
	
	auto pipeline_multisample_state_create_info = vk::PipelineMultisampleStateCreateInfo()
		.setRasterizationSamples(vk::SampleCountFlagBits::e1);

	auto pipeline_color_blent_attachment_state = vk::PipelineColorBlendAttachmentState()
		.setBlendEnable(true)
		.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
		.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
		.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
		.setAlphaBlendOp(vk::BlendOp::eAdd)
		.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

	auto pipeline_depth_stencil_state_create_info = vk::PipelineDepthStencilStateCreateInfo();

	auto pipeline_color_blend_state_create_info = vk::PipelineColorBlendStateCreateInfo()
		.setAttachmentCount(1)
		.setPAttachments(&pipeline_color_blent_attachment_state);

	auto dynamic_states = { 
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};

	auto pipeline_dynamic_state_create_info = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStates(dynamic_states);

	ImGui_ImplVulkan_CreatePipelineLayout(device);
	
	auto graphics_pipeline_create_info = vk::GraphicsPipelineCreateInfo()
		.setFlags(bd->PipelineCreateFlags)
		.setStages(pipeline_shader_stage_create_info)
		.setPVertexInputState(&pipeline_vertex_input_state_create_info)
		.setPInputAssemblyState(&pipeline_input_assembly_state_create_info)
		.setPViewportState(&pipeline_viewport_state_create_info)
		.setPRasterizationState(&pipeline_rasterization_state_create_info)
		.setPMultisampleState(&pipeline_multisample_state_create_info)
		.setPDepthStencilState(&pipeline_depth_stencil_state_create_info)
		.setPColorBlendState(&pipeline_color_blend_state_create_info)
		.setPDynamicState(&pipeline_dynamic_state_create_info)
		.setLayout(*bd->PipelineLayout)
		.setRenderPass(*renderPass)
		.setSubpass(subpass);

	pipeline = device.createGraphicsPipeline(pipelineCache, graphics_pipeline_create_info);
}

bool ImGui_ImplVulkan_CreateDeviceObjects()
{
	auto bd = ImGui_ImplVulkan_GetBackendData();
	auto v = &bd->VulkanInitInfo;
	
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

		bd->FontSampler = g_Device.createSampler(sampler_create_info);
	}
	{
		// TODO: this part of code are copy pasted

		auto descriptor_set_layout_binding = vk::DescriptorSetLayoutBinding()
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setDescriptorCount(1)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment)
			.setPImmutableSamplers(&*bd->FontSampler);

		auto descriptor_set_layout_create_info = vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(1)
			.setPBindings(&descriptor_set_layout_binding);

		bd->DescriptorSetLayout = g_Device.createDescriptorSetLayout(descriptor_set_layout_create_info);
	}
	{
		// Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
		auto push_constant_range = vk::PushConstantRange()
			.setStageFlags(vk::ShaderStageFlagBits::eVertex)
			.setOffset(0)
			.setSize(sizeof(float) * 4);

		auto pipeline_layout_create_info = vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(1)
			.setPSetLayouts(&*bd->DescriptorSetLayout)
			.setPushConstantRangeCount(1)
			.setPPushConstantRanges(&push_constant_range);

		bd->PipelineLayout = g_Device.createPipelineLayout(pipeline_layout_create_info);
	}

	ImGui_ImplVulkan_CreatePipeline(g_Device, g_PipelineCache, g_RenderPass, bd->Pipeline, bd->Subpass);

	return true;
}

void ImGui_ImplVulkan_DestroyDeviceObjects()
{
	auto bd = ImGui_ImplVulkan_GetBackendData();
	auto v = &bd->VulkanInitInfo;
	
	ImGui_ImplVulkanH_DestroyWindowRenderBuffers(*g_Device, bd->MainWindowRenderBuffers);
}

bool ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo* info)
{
	auto& io = ImGui::GetIO();
	IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

	auto bd = new ImGui_ImplVulkan_Data();
	io.BackendRendererUserData = (void*)bd;
	io.BackendRendererName = "imgui_impl_vulkan";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

	IM_ASSERT(info->MinImageCount >= 2);
	IM_ASSERT(info->ImageCount >= info->MinImageCount);

	bd->VulkanInitInfo = *info;
	bd->Subpass = info->Subpass;

	ImGui_ImplVulkan_CreateDeviceObjects();

	return true;
}

void ImGui_ImplVulkan_Shutdown()
{
	auto bd = ImGui_ImplVulkan_GetBackendData();
	IM_ASSERT(bd != NULL && "No renderer backend to shutdown, or already shutdown?");
	auto& io = ImGui::GetIO();

	ImGui_ImplVulkan_DestroyDeviceObjects();
	io.BackendRendererName = NULL;
	io.BackendRendererUserData = NULL;
	IM_DELETE(bd);
}

void ImGui_ImplVulkan_NewFrame()
{
	auto bd = ImGui_ImplVulkan_GetBackendData();
	IM_ASSERT(bd != NULL && "Did you call ImGui_ImplVulkan_Init()?");
	IM_UNUSED(bd);
}

void ImGui_ImplVulkan_SetMinImageCount(uint32_t min_image_count)
{
	auto bd = ImGui_ImplVulkan_GetBackendData();
	IM_ASSERT(min_image_count >= 2);
	
	if (bd->VulkanInitInfo.MinImageCount == min_image_count)
		return;

	auto v = &bd->VulkanInitInfo;
	g_Device.waitIdle();

	ImGui_ImplVulkanH_DestroyWindowRenderBuffers(*g_Device, bd->MainWindowRenderBuffers);
	bd->VulkanInitInfo.MinImageCount = min_image_count;
}

// Register a texture
// FIXME: This is experimental in the sense that we are unsure how to best design/tackle this problem, please post to https://github.com/ocornut/imgui/pull/914 if you have suggestions.
vk::raii::DescriptorSet ImGui_ImplVulkan_AddTexture(vk::Sampler sampler, vk::ImageView image_view, vk::ImageLayout image_layout)
{
	auto bd = ImGui_ImplVulkan_GetBackendData();
	auto v = &bd->VulkanInitInfo;

	auto descriptor_set_allocate_info = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(*g_DescriptorPool)
		.setDescriptorSetCount(1)
		.setPSetLayouts(&*bd->DescriptorSetLayout);

	auto descriptor_set = std::move(g_Device.allocateDescriptorSets(descriptor_set_allocate_info).at(0));

	auto descriptor_image_info = vk::DescriptorImageInfo()
		.setSampler(sampler)
		.setImageView(image_view)
		.setImageLayout(image_layout);

	auto write_descriptor_set = vk::WriteDescriptorSet()
		.setDstSet(*descriptor_set)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setPImageInfo(&descriptor_image_info);

	g_Device.updateDescriptorSets({ write_descriptor_set }, {});
	
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

vk::SurfaceFormatKHR ImGui_ImplVulkanH_SelectSurfaceFormat(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, std::vector<vk::Format> request_formats, vk::ColorSpaceKHR request_color_space)
{
	// Per Spec Format and View Format are expected to be the same unless VK_IMAGE_CREATE_MUTABLE_BIT was set at image creation
	// Assuming that the default behavior is without setting this bit, there is no need for separate Swapchain image and image view format
	// Additionally several new color spaces were introduced with Vulkan Spec v1.0.40,
	// hence we must make sure that a format with the mostly available color space, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, is found and used.

	auto avail_format = physical_device.getSurfaceFormatsKHR(surface);

	// First check if only one format, VK_FORMAT_UNDEFINED, is available, which would imply that any format is available
	if (avail_format.size() == 1)
	{
		if (avail_format.at(0).format == vk::Format::eUndefined)
		{
			vk::SurfaceFormatKHR ret;
			ret.format = request_formats.at(0);
			ret.colorSpace = request_color_space;
			return ret;
		}
		else
		{
			// No point in searching another format
			return avail_format.at(0);
		}
	}
	else
	{
		// Request several formats, the first found will be used
		for (int request_i = 0; request_i < request_formats.size(); request_i++)
			for (uint32_t avail_i = 0; avail_i < avail_format.size(); avail_i++)
				if (avail_format.at(avail_i).format == request_formats.at(request_i) && avail_format.at(avail_i).colorSpace == request_color_space)
					return avail_format.at(avail_i);

		// If none of the requested image formats could be found, use the first available
		return avail_format.at(0);
	}
}

vk::PresentModeKHR ImGui_ImplVulkanH_SelectPresentMode(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, const std::vector<vk::PresentModeKHR>& request_modes)
{
	auto avail_modes = physical_device.getSurfacePresentModesKHR(surface);

	for (int request_i = 0; request_i < (int)request_modes.size(); request_i++)
		for (uint32_t avail_i = 0; avail_i < avail_modes.size(); avail_i++)
			if (request_modes[request_i] == avail_modes[avail_i])
				return request_modes[request_i];

	return vk::PresentModeKHR::eFifo;
}

void ImGui_ImplVulkanH_CreateWindowCommandBuffers(vk::PhysicalDevice physical_device, vk::raii::Device& device, ImGui_ImplVulkanH_Window& wd, uint32_t queue_family)
{
	for (auto& frame : wd.Frames)
	{
		auto command_pool_create_info = vk::CommandPoolCreateInfo()
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.setQueueFamilyIndex(queue_family);

		frame.CommandPool = device.createCommandPool(command_pool_create_info);

		auto command_buffer_allocate_info = vk::CommandBufferAllocateInfo()
			.setCommandPool(*frame.CommandPool)
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandBufferCount(1);

		frame.CommandBuffer = std::move(device.allocateCommandBuffers(command_buffer_allocate_info).at(0));

		auto fence_create_info = vk::FenceCreateInfo()
			.setFlags(vk::FenceCreateFlagBits::eSignaled);

		frame.Fence = device.createFence(fence_create_info);

		auto semaphore_create_info = vk::SemaphoreCreateInfo();

		frame.ImageAcquiredSemaphore = device.createSemaphore(semaphore_create_info);
		frame.RenderCompleteSemaphore = device.createSemaphore(semaphore_create_info);
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
void ImGui_ImplVulkanH_CreateWindowSwapChain(vk::PhysicalDevice physical_device, vk::raii::Device& device, ImGui_ImplVulkanH_Window& wd, int w, int h, uint32_t min_image_count)
{
	device.waitIdle();

	wd.Frames.clear();

	// If min image count was not specified, request different count of images dependent on selected present mode
	if (min_image_count == 0)
		min_image_count = ImGui_ImplVulkanH_GetMinImageCountFromPresentMode(wd.PresentMode);

	// Create Swapchain
	{
		auto swapchain_create_info = vk::SwapchainCreateInfoKHR()
			.setSurface(*wd.Surface)
			.setMinImageCount(min_image_count)
			.setImageFormat(wd.SurfaceFormat.format)
			.setImageColorSpace(wd.SurfaceFormat.colorSpace)
			.setImageArrayLayers(1)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
			.setImageSharingMode(vk::SharingMode::eExclusive)
			.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
			.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
			.setPresentMode(wd.PresentMode)
			.setClipped(true)
			.setOldSwapchain(*wd.Swapchain);

		auto capabilities = physical_device.getSurfaceCapabilitiesKHR(*wd.Surface);

		if (swapchain_create_info.minImageCount < capabilities.minImageCount)
			swapchain_create_info.minImageCount = capabilities.minImageCount;
		else if (capabilities.maxImageCount != 0 && swapchain_create_info.minImageCount > capabilities.maxImageCount)
			swapchain_create_info.minImageCount = capabilities.maxImageCount;

		if (capabilities.currentExtent.width == 0xffffffff)
		{
			swapchain_create_info.imageExtent.width = wd.Width = w;
			swapchain_create_info.imageExtent.height = wd.Height = h;
		}
		else
		{
			swapchain_create_info.imageExtent.width = wd.Width = capabilities.currentExtent.width;
			swapchain_create_info.imageExtent.height = wd.Height = capabilities.currentExtent.height;
		}
		
		wd.Swapchain = device.createSwapchainKHR(swapchain_create_info);
		
		auto backbuffers = wd.Swapchain.getImages();

		IM_ASSERT(wd.Frames.empty());
		wd.Frames.resize(backbuffers.size());
		
		for (uint32_t i = 0; i < wd.Frames.size(); i++)
			wd.Frames[i].Backbuffer = backbuffers[i];
	}

	auto attachment_description = vk::AttachmentDescription()
		.setFormat(wd.SurfaceFormat.format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
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

	g_RenderPass = device.createRenderPass(render_pass_create_info);
		
	for (auto& frame : wd.Frames)
	{
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
			.setFormat(wd.SurfaceFormat.format)
			.setComponents(image_view_component_mapping)
			.setSubresourceRange(image_view_subresource_range)
			.setImage(frame.Backbuffer);

		frame.BackbufferView = device.createImageView(image_view_create_info);

		auto framebuffer_create_info = vk::FramebufferCreateInfo()
			.setRenderPass(*g_RenderPass)
			.setAttachmentCount(1)
			.setPAttachments(&*frame.BackbufferView)
			.setWidth(wd.Width)
			.setHeight(wd.Height)
			.setLayers(1);

		frame.Framebuffer = device.createFramebuffer(framebuffer_create_info);
	}
}

// Create or resize window
void ImGui_ImplVulkanH_CreateOrResizeWindow(vk::Instance instance, vk::PhysicalDevice physical_device, vk::raii::Device& device, ImGui_ImplVulkanH_Window& wd, uint32_t queue_family, int width, int height, uint32_t min_image_count)
{
	ImGui_ImplVulkanH_CreateWindowSwapChain(physical_device, device, wd, width, height, min_image_count);
	ImGui_ImplVulkanH_CreateWindowCommandBuffers(physical_device, device, wd, queue_family);
}

void ImGui_ImplVulkanH_DestroyWindow(vk::Instance instance, vk::Device device, ImGui_ImplVulkanH_Window& wd)
{
	device.waitIdle(); // FIXME: We could wait on the Queue if we had the queue in wd-> (otherwise VulkanH functions can't use globals)
	wd.Frames.clear();
	wd = ImGui_ImplVulkanH_Window();
}

void ImGui_ImplVulkanH_DestroyWindowRenderBuffers(vk::Device device, ImGui_ImplVulkanH_WindowRenderBuffers& buffers)
{
	buffers.FrameRenderBuffers.clear();
	buffers.Index = 0;
}
