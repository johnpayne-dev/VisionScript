#include <stdlib.h>
#include "VertexBuffer.h"
#include "Graphics.h"

VertexLayout VertexLayoutCreate(int32_t attributeCount, VertexAttribute * attributes, int32_t * offsets)
{
	VertexLayout layout = (VertexLayout)
	{
		.attributeCount = attributeCount,
		.attributes = malloc(attributeCount * sizeof(VkVertexInputAttributeDescription)),
	};
	
	// calculate vertex size
	int32_t size = 0;
	for (int32_t i = 0; i < attributeCount; i++)
	{
		layout.attributes[i] = (VkVertexInputAttributeDescription)
		{
			.binding = 0,
			.format = (VkFormat)attributes[i],
			.location = i,
			.offset = offsets == NULL ? size : offsets[i],
		};
		switch (attributes[i])
		{
			case VertexAttributeFloat: size += sizeof(float); break;
			case VertexAttributeVec2: size += 2 * sizeof(float); break;
			case VertexAttributeVec3: size += 3 * sizeof(float); break;
			case VertexAttributeVec4: size += 4 * sizeof(float); break;
			case VertexAttributeByte4: size += 4; break;
		}
	}
	
	// create vertex description
	layout.binding = (VkVertexInputBindingDescription)
	{
		.binding = 0,
		.stride = offsets == NULL ? size : offsets[attributeCount],
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};
	layout.size = layout.binding.stride;
	
	return layout;
}

void VertexLayoutFree(VertexLayout layout)
{
	free(layout.attributes);
}

VertexBuffer VertexBufferCreate(VertexLayout layout, int32_t vertexCount, int32_t indexCount)
{
	VertexBuffer vertexBuffer = (VertexBuffer)
	{
		.layout = layout,
		.vertexCount = vertexCount,
		.indexCount = indexCount,
	};
	
	uint64_t size = vertexCount * layout.size + indexCount * sizeof(int32_t);
	
	// create the staging buffer
	VkBufferCreateInfo stagingInfo =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};
	VmaAllocationCreateInfo stagingAllocationInfo =
	{
		.usage = VMA_MEMORY_USAGE_CPU_ONLY,
	};
	vmaCreateBuffer(graphics.allocator, &stagingInfo, &stagingAllocationInfo, &vertexBuffer.stagingBuffer, &vertexBuffer.stagingAllocation, NULL);
	
	// create the GPU buffer
	VkBufferCreateInfo bufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};
	if (indexCount > 0) { bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT; }
	VmaAllocationCreateInfo allocationInfo =
	{
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
	};
	vmaCreateBuffer(graphics.allocator, &bufferInfo, &allocationInfo, &vertexBuffer.vertexBuffer, &vertexBuffer.vertexAllocation, NULL);
	
	// create the command buffer
	VkCommandBufferAllocateInfo commandAllocateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = graphics.commandPool,
		.commandBufferCount = 1,
	};
	vkAllocateCommandBuffers(graphics.device, &commandAllocateInfo, &vertexBuffer.commandBuffer);
	
	// create the upload fence
	VkFenceCreateInfo fenceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};
	vkCreateFence(graphics.device, &fenceInfo, NULL, &vertexBuffer.uploadFence);
	
	return vertexBuffer;
}

void * VertexBufferMapVertices(VertexBuffer vertexBuffer, uint32_t ** indices)
{
	// grab the pointer to the staging buffer
	void * data;
	vmaMapMemory(graphics.allocator, vertexBuffer.stagingAllocation, &data);
	
	// if the buffer uses indices then also grab the pointer to the index area in the staging buffer
	if (vertexBuffer.indexCount > 0 && indices != NULL)
	{
		*indices = (uint32_t *)((uint8_t *)data + vertexBuffer.vertexCount * vertexBuffer.layout.size);
	}
	return data;
}

void VertexBufferUnmapVertices(VertexBuffer vertexBuffer)
{
	vmaUnmapMemory(graphics.allocator, vertexBuffer.stagingAllocation);
}

void VertexBufferUpload(VertexBuffer vertexBuffer)
{
	// wait for fence in case the vertex buffer hasn't finished uploading from a previous call
	vkWaitForFences(graphics.device, 1, &vertexBuffer.uploadFence, VK_TRUE, UINT64_MAX);
	vkResetFences(graphics.device, 1, &vertexBuffer.uploadFence);
	
	// begin recording commands
	VkCommandBufferBeginInfo beginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(vertexBuffer.commandBuffer, &beginInfo);
	
	// copy the staging buffer to the GPU buffer
	VkBufferCopy copyInfo =
	{
		.srcOffset = 0,
		.dstOffset = 0,
		.size = vertexBuffer.vertexCount * vertexBuffer.layout.size + 4 * vertexBuffer.indexCount,
	};
	vkCmdCopyBuffer(vertexBuffer.commandBuffer, vertexBuffer.stagingBuffer, vertexBuffer.vertexBuffer, 1, &copyInfo);
	
	// submit the command buffer
	vkEndCommandBuffer(vertexBuffer.commandBuffer);
	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &vertexBuffer.commandBuffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = NULL,
	};
	vkQueueSubmit(graphics.graphicsQueue, 1, &submitInfo, vertexBuffer.uploadFence);
}

void VertexBufferFree(VertexBuffer vertexBuffer)
{
	vkWaitForFences(graphics.device, 1, &vertexBuffer.uploadFence, VK_TRUE, UINT64_MAX);
	vkDestroyFence(graphics.device, vertexBuffer.uploadFence, NULL);
	vkFreeCommandBuffers(graphics.device, graphics.commandPool, 1, &vertexBuffer.commandBuffer);
	vmaDestroyBuffer(graphics.allocator, vertexBuffer.stagingBuffer, vertexBuffer.stagingAllocation);
	vmaDestroyBuffer(graphics.allocator, vertexBuffer.vertexBuffer, vertexBuffer.vertexAllocation);
}
