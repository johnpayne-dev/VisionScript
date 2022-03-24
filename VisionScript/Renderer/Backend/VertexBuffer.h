#ifndef VertexBuffer_h
#define VertexBuffer_h

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

typedef enum VertexAttribute
{
	VertexAttributeFloat = VK_FORMAT_R32_SFLOAT,
	VertexAttributeVec2 = VK_FORMAT_R32G32_SFLOAT,
	VertexAttributeVec3 = VK_FORMAT_R32G32B32_SFLOAT,
	VertexAttributeVec4 = VK_FORMAT_R32G32B32A32_SFLOAT,
	VertexAttributeByte4 = VK_FORMAT_R8G8B8A8_UNORM,
} VertexAttribute;

typedef struct VertexLayout
{
	VkVertexInputBindingDescription binding;
	uint32_t attributeCount;
	VkVertexInputAttributeDescription * attributes;
	uint32_t size;
} VertexLayout;

VertexLayout VertexLayoutCreate(int32_t attributeCount, VertexAttribute attributes[], int32_t * offsets);
void VertexLayoutFree(VertexLayout layout);

typedef struct VertexBuffer
{
	VertexLayout layout;
	int32_t vertexCount;
	int32_t indexCount;
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VkBuffer vertexBuffer;
	VmaAllocation vertexAllocation;
	VkCommandBuffer commandBuffer;
	VkFence uploadFence;
} VertexBuffer;

VertexBuffer VertexBufferCreate(VertexLayout vertexLayout, int32_t vertexCount, int32_t indexCount);

void * VertexBufferMapVertices(VertexBuffer vertexBuffer, uint32_t ** indices);

void VertexBufferUnmapVertices(VertexBuffer vertexBuffer);

void VertexBufferUpload(VertexBuffer vertexBuffer);

void VertexBufferFree(VertexBuffer vertexBuffer);

#endif
