#ifndef Graphics_h
#define Graphics_h

#include <stdbool.h>
#include <vulkan/vulkan.h>

#define FRAMES_IN_FLIGHT 3

static struct Graphics
{
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	bool computeShadersSupported;
	int32_t computeQueueIndex, graphicsQueueIndex, presentQueueIndex;
	VkQueue computeQueue, graphicsQueue, presentQueue;
	VkRenderPass renderPass;
	VkCommandPool commandPool;
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
		uint32_t imageCount;
		VkImage * images;
		uint32_t imageIndex;
	} swapchain;
} graphics;

void GraphicsInitialize(int32_t width, int32_t height);

void GraphicsRecreateSwapchain(int32_t width, int32_t height);

void GraphicsUpdate(void);

void GraphicsStartCompute(void);

void GraphicsEndCompute(void);

void GraphicsAquireNextSwapchainImage(void);

void GraphicsPresentSwapchainImage(void);

void GraphicsShutdown(void);

#endif
