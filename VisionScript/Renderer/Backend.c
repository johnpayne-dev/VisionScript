#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <vulkan/vulkan.h>
#include "Backend.h"
#include "Application.h"

#ifdef __APPLE__
	#include <vulkan/vulkan_macos.h>
	static const char * requiredExtensionNames[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_MVK_MACOS_SURFACE_EXTENSION_NAME };
	static uint32_t requiredExtensionCount = sizeof(requiredExtensionNames) / sizeof(requiredExtensionNames[0]);
#endif

static VkInstance instance;
static VkSurfaceKHR surface;
static VkPhysicalDevice physicalDevice;
static VkDevice device;
static bool computeShadersSupported;
static int32_t computeQueueIndex, graphicsQueueIndex, presentQueueIndex;
static VkQueue computeQueue, graphicsQueue, presentQueue;
static VkRenderPass renderPass;
static VkCommandPool commandPool;
static const int32_t frameCount = 3;
static struct FrameInFlight
{
	VkCommandBuffer commandBuffer;
	VkSemaphore imageAvailable;
	VkSemaphore renderFinished;
	VkFence frameReady;
	VkCommandBuffer computeCommandBuffer;
	VkSemaphore computeFinished;
	VkFence computeFence;
} frames[frameCount];
static int32_t frameIndex;
static struct Swapchain
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

static void CheckExtensionSupport()
{
	// get the list of supported extensions
	uint32_t supportedExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &supportedExtensionCount, NULL);
	VkExtensionProperties * supportedExtensions = malloc(supportedExtensionCount * sizeof(VkExtensionProperties));
	vkEnumerateInstanceExtensionProperties(NULL, &supportedExtensionCount, supportedExtensions);
	
	// make sure that all the required extensions are supported
	for (int32_t i = 0; i < requiredExtensionCount; i++)
	{
		bool extensionSupported = false;
		for (int32_t j = 0; j < supportedExtensionCount; j++)
		{
			if (strcmp(supportedExtensions[j].extensionName, requiredExtensionNames[i]) == 0)
			{
				extensionSupported = true;
				break;
			}
		}
		if (!extensionSupported) { printf("[Fatal] trying to initialize Vulkan, but a required extension is not supported: %s\n", requiredExtensionNames[i]); }
	}
	
	free(supportedExtensions);
}

static bool CheckValidationLayerSupport()
{
	// get the list of layers
	uint32_t availableLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);
	VkLayerProperties * availableLayers = malloc(availableLayerCount * sizeof(VkLayerProperties));
	vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers);
	
	// check if VK_LAYER_KHRONOS_validation is supported
	bool supported = false;
	for (int32_t i = 0; i < availableLayerCount; i++)
	{
		if (strcmp(availableLayers[i].layerName, "VK_LAYER_KHRONOS_validation") == 0)
		{
			supported = true;
		}
	}
	
	free(availableLayers);
	return supported;
}

static void CreateInstance(bool useValidations)
{
	// check if validation layers are supported
	bool validationsSupported = CheckValidationLayerSupport();
	if (useValidations && !validationsSupported) { printf("[Warning] trying to use validation layers but they are not supported\n"); }
	useValidations = useValidations && validationsSupported;
	const char * validationLayer = "VK_LAYER_KHRONOS_validation";
	
	// setup instance info
	VkApplicationInfo appInfo =
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "VisionScript",
		.applicationVersion = VK_MAKE_VERSION(0, 0, 0),
		.pEngineName = "VSE",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0,
	};
	VkInstanceCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = requiredExtensionCount,
		.ppEnabledExtensionNames = (const char * const *)requiredExtensionNames,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
	};
	if (useValidations)
	{
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = &validationLayer;
	}
	
	// create VkInstance
	VkResult result = vkCreateInstance(&createInfo, NULL, &instance);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to initialize Vulkan, but failed to create VkInstance: %i\n", result); }
}

static void CreateSurface()
{
#ifdef __APPLE__
	PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK = (PFN_vkCreateMacOSSurfaceMVK)vkGetInstanceProcAddr((VkInstance)instance, "vkCreateMacOSSurfaceMVK");
	VkMacOSSurfaceCreateInfoMVK createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
		.pView = ApplicationMacOSView(),
	};
	VkResult result = vkCreateMacOSSurfaceMVK(instance, &createInfo, NULL, &surface);
#endif
	if (result != VK_SUCCESS) { printf("[Fatal] trying to initialize Vulkan, but failed to create VkSurfaceKHR\n"); }
}

static void ChoosePhysicalDevice()
{
	physicalDevice = VK_NULL_HANDLE;
	
	// get the list of available GPUs
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
	if (deviceCount == 0) { printf("[Fatal] Trying to initialize Vulkan, but there is no GPU with Vulkan support\n"); }
	VkPhysicalDevice * devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
	
	// default as the first available gpu
	physicalDevice = devices[0];
	
	// check each gpu to make sure it is suitable
	// the requirements for a suitable gpu are the availability of a graphics and present queue (compute is optional) and the support for a swapchain.
	// will prefer a dedicated gpu
	bool graphicsFound = false, presentFound = false, computeFound = false;
	int32_t graphics = 0, present = 0, compute = 0;
	bool foundSuitableDevice = false;
	for (int32_t i = 0; i < deviceCount; i++)
	{
		// get the list of queue family properties
		unsigned int queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, NULL);
		VkQueueFamilyProperties * queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies);
		
		// check each queue family to see if it supports any of the desired queues
		for (int32_t j = 0; j < queueFamilyCount; j++)
		{
			if (queueFamilies[j].queueCount > 0 && queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT && !graphicsFound)
			{
				graphicsFound = true;
				graphics = j;
			}
			if (queueFamilies[j].queueCount > 0 && queueFamilies[j].queueFlags & VK_QUEUE_COMPUTE_BIT && !computeFound)
			{
				computeFound = true;
				compute = j;
			}
			VkBool32 presentSupported = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, surface, &presentSupported);
			if (queueFamilies[j].queueCount > 0 && presentSupported && !presentFound)
			{
				presentFound = true;
				present = j;
			}
		}
		free(queueFamilies);
		
		// check if swapchains are supported by iterating through the list of device extensions
		bool swapchainSupported = false;
		uint32_t availableExtensionCount;
		vkEnumerateDeviceExtensionProperties(devices[i], NULL, &availableExtensionCount, NULL);
		VkExtensionProperties * availableExtensions = malloc(availableExtensionCount * sizeof(VkExtensionProperties));
		vkEnumerateDeviceExtensionProperties(devices[i], NULL, &availableExtensionCount, availableExtensions);
		for (int32_t i = 0; i < availableExtensionCount; i++)
		{
			if (strcmp(availableExtensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
			{
				swapchainSupported = true;
			}
		}
		free(availableExtensions);
		
		// check if everything needed is supported
		if (graphicsFound && presentFound && swapchainSupported)
		{
			foundSuitableDevice = true;
			computeShadersSupported = computeFound;
			computeQueueIndex = compute;
			graphicsQueueIndex = graphics;
			presentQueueIndex = present;
			physicalDevice = devices[i];
			
			// if the device is a dedicated gpu then stop search immediately
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { break; }
		}
	}
	if (!foundSuitableDevice) { printf("[Fatal] trying to initialize Vulkan, but there is no suitable device found for Vulkan\n"); }
	
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	printf("[Info] using graphics device: %s\n", deviceProperties.deviceName);
	free(devices);
}

static void CreateLogicalDevice()
{
	// create an array of the three QueueCreateInfos
	VkDeviceQueueCreateInfo queueInfos[3];
	int32_t queueCount = 0;
	
	VkDeviceQueueCreateInfo graphicsQueueInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = graphicsQueueIndex,
		.queueCount = 1,
		.pQueuePriorities = &(float){ 1.0 },
	};
	queueInfos[queueCount++] = graphicsQueueInfo;
	
	VkDeviceQueueCreateInfo presentQueueInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = presentQueueIndex,
		.queueCount = 1,
		.pQueuePriorities = &(float){ 1.0 },
	};
	if (presentQueueIndex != graphicsQueueIndex) { queueInfos[queueCount++] = presentQueueInfo; }
	
	if (computeShadersSupported)
	{
		VkDeviceQueueCreateInfo computeQueueInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = computeQueueIndex,
			.queueCount = 1,
			.pQueuePriorities = &(float){ 1.0 },
		};
		if (computeQueueIndex != graphicsQueueIndex && computeQueueIndex != presentQueueIndex) { queueInfos[queueCount++] = computeQueueInfo; }
	}
	
	// enable specific device features that are required (both features enabled below are supported by all GPUs that support Vulkan)
	VkPhysicalDeviceFeatures deviceFeatures =
	{
		.fillModeNonSolid = true,
		.samplerAnisotropy = true,
	};
	const char * extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	
	// create the VkDevice
	VkDeviceCreateInfo deviceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = queueCount,
		.pQueueCreateInfos = queueInfos,
		.pEnabledFeatures = &deviceFeatures,
		.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]),
		.ppEnabledExtensionNames = extensions,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
	};
	VkResult result = vkCreateDevice(physicalDevice, &deviceInfo, NULL, &device);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to initialize Vulkan, but failed to create VkDevice: %i\n", result); }
	
	// get the queues that were automatically created after creating the VkDevice
	vkGetDeviceQueue(device, graphicsQueueIndex, 0, &graphicsQueue);
	vkGetDeviceQueue(device, presentQueueIndex, 0, &presentQueue);
	if (computeShadersSupported) { vkGetDeviceQueue(device, computeQueueIndex, 0, &computeQueue); }
}

static void CreateRenderPass()
{
	// create the single render pass that is used for all drawing operations
	VkAttachmentDescription colorAttachment =
	{
		.format = VK_FORMAT_R8G8B8A8_UNORM,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
		.initialLayout = VK_IMAGE_LAYOUT_GENERAL,
		.finalLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	VkAttachmentReference colorAttachmentReference =
	{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	
	VkAttachmentDescription depthAttachment =
	{
		.format = VK_FORMAT_D32_SFLOAT_S8_UINT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
		.initialLayout = VK_IMAGE_LAYOUT_GENERAL,
		.finalLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	VkAttachmentReference depthAttachmentReference =
	{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};
	
	VkSubpassDescription subpass =
	{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentReference,
		.pDepthStencilAttachment = &depthAttachmentReference,
	};
	
	VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = sizeof(attachments) / sizeof(attachments[0]),
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 0,
		.pDependencies = NULL,
	};
	VkResult result = vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to initialize Graphics, but failed to create VkRenderPass: %i\n", result); }
}

static void CreateCommandPool()
{
	// create the command pool that's used to allocate all of the command buffers
	VkCommandPoolCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = graphicsQueueIndex,
	};
	VkResult result = vkCreateCommandPool(device, &createInfo, NULL, &commandPool);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to initialize Graphics, but failed to create VkCommandPool: %i\n", result); }
}

static void CreateFramesInFlight()
{
	// create the frame resources that allow for asynchronous rendering
	for (int32_t i = 0; i < frameCount; i++)
	{
		// allocate the command buffers
		VkCommandBufferAllocateInfo allocateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandPool = commandPool,
			.commandBufferCount = 1,
		};
		vkAllocateCommandBuffers(device, &allocateInfo, &frames[i].commandBuffer);
		vkAllocateCommandBuffers(device, &allocateInfo, &frames[i].computeCommandBuffer);
		
		// create the semaphores that sync the frame resources with each other on the GPU
		VkSemaphoreCreateInfo semaphoreInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};
		vkCreateSemaphore(device, &semaphoreInfo, NULL, &frames[i].imageAvailable);
		vkCreateSemaphore(device, &semaphoreInfo, NULL, &frames[i].renderFinished);
		vkCreateSemaphore(device, &semaphoreInfo, NULL, &frames[i].computeFinished);
		
		// create the fences that sync the GPU with the CPU
		VkFenceCreateInfo fenceInfo =
		{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};
		vkCreateFence(device, &fenceInfo, NULL, &frames[i].frameReady);
		vkCreateFence(device, &fenceInfo, NULL, &frames[i].computeFence);
	}
	frameIndex = 0;
}

static void CreateSwapchain(int32_t width, int32_t height)
{
	swapchain.targetExtent = (VkExtent2D){ .width = width, .height = height };
	
	// get the available capabilities of the window surface
	VkSurfaceCapabilitiesKHR availableCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &availableCapabilities);
	
	// get the list of supported surface formats
	uint32_t availableFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &availableFormatCount, NULL);
	VkSurfaceFormatKHR * availableFormats = malloc(availableFormatCount * sizeof(VkSurfaceFormatKHR));
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &availableFormatCount, availableFormats);
	
	// set the surface format to VK_FORMAT_B8G8R8A8_UNORM if it's supported
	VkSurfaceFormatKHR surfaceFormat = availableFormats[0];
	VkFormat targetFormat = VK_FORMAT_B8G8R8A8_UNORM;
	for (int32_t i = 0; i < availableFormatCount; i++)
	{
		if (availableFormats[i].format == targetFormat)
		{
			surfaceFormat = availableFormats[i];
		}
	}
	free(availableFormats);
	
	// get the list of supported present modes
	uint32_t availablePresentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &availablePresentModeCount, NULL);
	VkPresentModeKHR * availablePresentModes = malloc(availablePresentModeCount * sizeof(VkPresentModeKHR));
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &availablePresentModeCount, availablePresentModes);
	
	// set the present mode to the target present mode if it's available (VK_PRESENT_MODE_FIFO_KHR is guarenteed to be supported)
	swapchain.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (int32_t i = 0; i < availablePresentModeCount; i++)
	{
		if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) { swapchain.presentMode = availablePresentModes[i]; }
	}
	free(availablePresentModes);
	
	// set the window size
	VkExtent2D extent =
	{
		.width = (uint32_t)width,
		.height = (uint32_t)height,
	};
	extent.width = fmaxf(availableCapabilities.minImageExtent.width, fminf(availableCapabilities.maxImageExtent.width, extent.width));
	extent.height = fmaxf(availableCapabilities.minImageExtent.height, fminf(availableCapabilities.maxImageExtent.height, extent.height));
	if (extent.width != width || extent.height != height)
	{
		printf("[Warning] tried to create swapchain with dimensions (%i, %i) but (%i, %i) was chosen instead\n", width, height, extent.width, extent.height);
	}
	
	// target the number of swapchain images to 3
	uint32_t imageCount = fmaxf(availableCapabilities.minImageCount, fminf(3, availableCapabilities.maxImageCount));
	printf("[Info] initializing swapchain with %i images\n", imageCount);
	
	// setup swapchain create info
	VkSwapchainCreateInfoKHR createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.preTransform = availableCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = swapchain.presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};
	const uint32_t queueFamilyIndices[] = { graphicsQueueIndex, presentQueueIndex };
	if (graphicsQueueIndex != presentQueueIndex)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
		createInfo.queueFamilyIndexCount = 2;
	}
	else { createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; }
	
	// Create the swapchain
	VkResult result = vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain.instance);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to create swapchain, but failed to create VkSwapchainKHR: %i\n", result); }

	swapchain.extent = extent;
	swapchain.colorFormat = surfaceFormat.format;
}

static void GetSwapchainImages()
{
	// get the list of swapchain images
	vkGetSwapchainImagesKHR(device, swapchain.instance, &swapchain.imageCount, NULL);
	swapchain.images = malloc(swapchain.imageCount * sizeof(VkImage));
	vkGetSwapchainImagesKHR(device, swapchain.instance, &swapchain.imageCount, swapchain.images);
	
	// allocate a commandbuffer to queue the layout conversion of each iamge
	VkCommandBuffer commandBuffer;
	VkCommandBufferAllocateInfo commandAllocateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = commandPool,
		.commandBufferCount = 1,
	};
	vkAllocateCommandBuffers(device, &commandAllocateInfo, &commandBuffer);
	VkCommandBufferBeginInfo beginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	
	// record the commands that convert each image layout to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	for (int32_t i = 0; i < swapchain.imageCount; i++)
	{
		VkImageMemoryBarrier memoryBarrier =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = swapchain.images[i],
			.subresourceRange =
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.srcAccessMask = 0,
			.dstAccessMask = 0,
		};
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &memoryBarrier);
	}
	vkEndCommandBuffer(commandBuffer);
	
	// submit the command buffer
	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
	};
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkDeviceWaitIdle(device);
	
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void InitializeGraphicsBackend(int32_t width, int32_t height)
{
	printf("[Info] initializing the graphics backend...\n");
	CheckExtensionSupport();
	CreateInstance(true);
	CreateSurface();
	ChoosePhysicalDevice();
	CreateLogicalDevice();
	CreateRenderPass();
	CreateCommandPool();
	CreateFramesInFlight();
	CreateSwapchain(width, height);
	GetSwapchainImages();
	printf("[Info] successfully initialized the graphics backend\n");
}

void RecreateSwapchain(int32_t width, int32_t height)
{
	// destroy old swapchain
	vkDeviceWaitIdle(device);
	free(swapchain.images);
	vkDestroySwapchainKHR(device, swapchain.instance, NULL);
	
	printf("[Info] creating the swapchain...\n");
	CreateSwapchain(width, height);
	GetSwapchainImages();
	printf("[Info] successfully created the swapchain\n");
}

void UpdateBackend()
{
	// advance to the next frame resource
	frameIndex = (frameIndex + 1) % frameCount;
	vkWaitForFences(device, 1, &frames[frameIndex].frameReady, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &frames[frameIndex].frameReady);
}

void StartCompute()
{
	// sync the frame resource from last compute operation
	vkWaitForFences(device, 1, &frames[frameIndex].computeFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &frames[frameIndex].computeFence);
	
	// begin recording commands for the next compute operation
	vkResetCommandBuffer(frames[frameIndex].computeCommandBuffer, 0);
	VkCommandBufferBeginInfo beginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(frames[frameIndex].computeCommandBuffer, &beginInfo);
}

void EndCompute()
{
	// end command recording for the compute operation
	VkResult result = vkEndCommandBuffer(frames[frameIndex].computeCommandBuffer);
	if (result != VK_SUCCESS)
	{
		printf("[Fatal] trying to end compute recording, but failed to record compute command buffer: %i\n", result);
	}
	
	// submit command buffer
	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = NULL,
		.pWaitDstStageMask = NULL,
		.commandBufferCount = 1,
		.pCommandBuffers = &frames[frameIndex].computeCommandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &frames[frameIndex].computeFinished,
	};
	result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, frames[frameIndex].computeFence);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to send compute operations, but failed to submit queue: %i\n", result); }
	
	// add the compute operation to the list of prerender semaphores
	//Graphics.PreRenderSemaphores = ListPush(Graphics.PreRenderSemaphores, &Graphics.FrameResources[Graphics.FrameIndex].ComputeFinished);
}

void AquireNextSwapchainImage()
{
	// acquire next image (and try again if unsuccesful)
	VkResult result = vkAcquireNextImageKHR(device, swapchain.instance, UINT64_MAX, frames[frameIndex].imageAvailable, VK_NULL_HANDLE, &swapchain.imageIndex);
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) { printf("[Warning] unsuccessful aquire image: %i, trying again until successful...\n", result); }
	while (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		result = vkAcquireNextImageKHR(device, swapchain.instance, UINT64_MAX, frames[frameIndex].imageAvailable, VK_NULL_HANDLE, &swapchain.imageIndex);
		if (result == VK_SUCCESS) { printf("[Info] Succefully aquired image, continuing operations\n"); }
	}
	
	// begin recording commands
	vkResetCommandBuffer(frames[frameIndex].commandBuffer, 0);
	VkCommandBufferBeginInfo beginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	result = vkBeginCommandBuffer(frames[frameIndex].commandBuffer, &beginInfo);
}

void PresentSwapchainImage()
{
	// end command recording
	VkResult result = vkEndCommandBuffer(frames[frameIndex].commandBuffer);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to end graphics recording, but failed to record command buffer: %i\n", result); }
	
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	VkSemaphore waitSemaphores[] = { frames[frameIndex].imageAvailable };
	
	// submit the command buffer to the graphics queue
	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = sizeof(waitSemaphores) / sizeof(waitSemaphores[0]),
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &frames[frameIndex].commandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &frames[frameIndex].renderFinished,
	};
	result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, frames[frameIndex].frameReady);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to send graphics commands, but failed to submit queue: %i\n", result); }
	
	// submit the swapchain to the present queue
	VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frames[frameIndex].renderFinished,
		.swapchainCount = 1,
		.pSwapchains = &swapchain.instance,
		.pImageIndices = &swapchain.imageIndex,
	};
	vkQueuePresentKHR(presentQueue, &presentInfo);
}
