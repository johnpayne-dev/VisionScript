#ifndef Graphics_h
#define Graphics_h

#define FRAMES_IN_FLIGHT 3

#include <stdbool.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "Pipeline.h"
#include "VertexBuffer.h"

extern struct Graphics
{
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	bool computeShadersSupported;
	int32_t computeQueueIndex, graphicsQueueIndex, presentQueueIndex;
	VkQueue computeQueue, graphicsQueue, presentQueue;
	VkCommandPool commandPool;
	VmaAllocator allocator;
	struct FrameInFlight
	{
		VkCommandBuffer commandBuffer;
		VkSemaphore imageAvailable;
		VkSemaphore renderFinished;
		VkFence frameReady;
		VkCommandBuffer computeCommandBuffer;
		VkSemaphore computeFinished;
		VkFence computeFence;
	} frames[FRAMES_IN_FLIGHT];
	int32_t frameIndex;
	struct Swapchain
	{
		VkSwapchainKHR instance;
		VkPresentModeKHR presentMode;
		VkExtent2D targetExtent;
		VkExtent2D extent;
		VkFormat colorFormat;
		VkRenderPass renderPass;
		uint32_t imageCount;
		VkImage * images;
		VkImageView * imageViews;
		VkFramebuffer * framebuffers;
		uint32_t imageIndex;
	} swapchain;
	Pipeline boundPipeline;
} graphics;

void GraphicsInitialize(int32_t width, int32_t height);

void GraphicsRecreateSwapchain(int32_t width, int32_t height);

void GraphicsUpdate(void);

void GraphicsBegin(void);

void GraphicsClearColor(float r, float g, float b, float a);

void GraphicsBindPipeline(Pipeline pipeline);

void GraphicsRenderVertexBuffer(VertexBuffer buffer);

void GraphicsEnd(void);

void GraphicsShutdown(void);

#endif
