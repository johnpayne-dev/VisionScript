#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "Graphics.h"
#include "Renderer/Application.h"

#ifdef __APPLE__
	#include <vulkan/vulkan_macos.h>
	static const char * requiredExtensionNames[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_MVK_MACOS_SURFACE_EXTENSION_NAME };
	static uint32_t requiredExtensionCount = sizeof(requiredExtensionNames) / sizeof(requiredExtensionNames[0]);
#endif

struct Graphics graphics = { 0 };

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
	VkResult result = vkCreateInstance(&createInfo, NULL, &graphics.instance);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to initialize Vulkan, but failed to create VkInstance: %i\n", result); }
}

static void CreateSurface()
{
#ifdef __APPLE__
	PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK = (PFN_vkCreateMacOSSurfaceMVK)vkGetInstanceProcAddr((VkInstance)graphics.instance, "vkCreateMacOSSurfaceMVK");
	VkMacOSSurfaceCreateInfoMVK createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
		.pView = ApplicationMacOSView(),
	};
	VkResult result = vkCreateMacOSSurfaceMVK(graphics.instance, &createInfo, NULL, &graphics.surface);
#endif
	if (result != VK_SUCCESS) { printf("[Fatal] trying to initialize Vulkan, but failed to create VkSurfaceKHR\n"); }
}

static void ChoosePhysicalDevice(int32_t msaa)
{
	graphics.physicalDevice = VK_NULL_HANDLE;
	
	// get the list of available GPUs
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(graphics.instance, &deviceCount, NULL);
	if (deviceCount == 0) { printf("[Fatal] Trying to initialize Vulkan, but there is no GPU with Vulkan support\n"); }
	VkPhysicalDevice * devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices(graphics.instance, &deviceCount, devices);
	
	// default as the first available gpu
	graphics.physicalDevice = devices[0];
	
	// check each gpu to make sure it is suitable
	// the requirements for a suitable gpu are the availability of a graphics and present queue (compute is optional) and the support for a swapchain.
	// will prefer a dedicated gpu
	bool graphicsFound = false, presentFound = false, computeFound = false;
	int32_t graphicsIndex = 0, presentIndex = 0, computeIndex = 0;
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
				graphicsIndex = j;
			}
			if (queueFamilies[j].queueCount > 0 && queueFamilies[j].queueFlags & VK_QUEUE_COMPUTE_BIT && !computeFound)
			{
				computeFound = true;
				computeIndex = j;
			}
			VkBool32 presentSupported = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, graphics.surface, &presentSupported);
			if (queueFamilies[j].queueCount > 0 && presentSupported && !presentFound)
			{
				presentFound = true;
				presentIndex = j;
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
			graphics.computeShadersSupported = computeFound;
			graphics.computeQueueIndex = computeIndex;
			graphics.graphicsQueueIndex = graphicsIndex;
			graphics.presentQueueIndex = presentIndex;
			graphics.physicalDevice = devices[i];
			
			// if the device is a dedicated gpu then stop search immediately
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { break; }
		}
	}
	if (!foundSuitableDevice) { printf("[Fatal] trying to initialize Vulkan, but there is no suitable device found for Vulkan\n"); }
	
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(graphics.physicalDevice, &deviceProperties);
	printf("[Info] using graphics device: %s\n", deviceProperties.deviceName);
	
	// get max msaa samples available
	vkGetPhysicalDeviceProperties(graphics.physicalDevice, &deviceProperties);
	VkSampleCountFlagBits maxSamples = VK_SAMPLE_COUNT_1_BIT;
	VkSampleCountFlags counts = deviceProperties.limits.framebufferColorSampleCounts & deviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { maxSamples = VK_SAMPLE_COUNT_64_BIT; }
	else if (counts & VK_SAMPLE_COUNT_32_BIT) { maxSamples = VK_SAMPLE_COUNT_32_BIT; }
	else if (counts & VK_SAMPLE_COUNT_16_BIT) { maxSamples = VK_SAMPLE_COUNT_16_BIT; }
	else if (counts & VK_SAMPLE_COUNT_8_BIT) { maxSamples = VK_SAMPLE_COUNT_8_BIT; }
	else if (counts & VK_SAMPLE_COUNT_4_BIT) { maxSamples = VK_SAMPLE_COUNT_4_BIT; }
	else if (counts & VK_SAMPLE_COUNT_2_BIT) { maxSamples = VK_SAMPLE_COUNT_2_BIT; }
	
	graphics.samples = msaa;
	if (graphics.samples > maxSamples)
	{
		graphics.samples = maxSamples;
		printf("[Warning] requested number of samples not suppported\n");
	}
	
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
		.queueFamilyIndex = graphics.graphicsQueueIndex,
		.queueCount = 1,
		.pQueuePriorities = &(float){ 1.0 },
	};
	queueInfos[queueCount++] = graphicsQueueInfo;
	
	VkDeviceQueueCreateInfo presentQueueInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = graphics.presentQueueIndex,
		.queueCount = 1,
		.pQueuePriorities = &(float){ 1.0 },
	};
	if (graphics.presentQueueIndex != graphics.graphicsQueueIndex) { queueInfos[queueCount++] = presentQueueInfo; }
	
	if (graphics.computeShadersSupported)
	{
		VkDeviceQueueCreateInfo computeQueueInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = graphics.computeQueueIndex,
			.queueCount = 1,
			.pQueuePriorities = &(float){ 1.0 },
		};
		if (graphics.computeQueueIndex != graphics.graphicsQueueIndex && graphics.computeQueueIndex != graphics.presentQueueIndex) { queueInfos[queueCount++] = computeQueueInfo; }
	}
	
	// enable specific device features that are required
	VkPhysicalDeviceFeatures deviceFeatures =
	{
		.fillModeNonSolid = true,
		.largePoints = true,
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
	VkResult result = vkCreateDevice(graphics.physicalDevice, &deviceInfo, NULL, &graphics.device);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to initialize Vulkan, but failed to create VkDevice: %i\n", result); }
	
	// get the queues that were automatically created after creating the VkDevice
	vkGetDeviceQueue(graphics.device, graphics.graphicsQueueIndex, 0, &graphics.graphicsQueue);
	vkGetDeviceQueue(graphics.device, graphics.presentQueueIndex, 0, &graphics.presentQueue);
	if (graphics.computeShadersSupported) { vkGetDeviceQueue(graphics.device, graphics.computeQueueIndex, 0, &graphics.computeQueue); }
}

static void CreateCommandPool()
{
	// create the command pool that's used to allocate all of the command buffers
	VkCommandPoolCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = graphics.graphicsQueueIndex,
	};
	VkResult result = vkCreateCommandPool(graphics.device, &createInfo, NULL, &graphics.commandPool);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to initialize Graphics, but failed to create VkCommandPool: %i\n", result); }
}

static void CreateAllocator()
{
	// create the allocator that's used to manage any GPU memory allocations
	VmaAllocatorCreateInfo allocatorInfo =
	{
		.physicalDevice = graphics.physicalDevice,
		.device = graphics.device,
		.preferredLargeHeapBlockSize = 16 * 1024 * 1024, // 16mb
		.instance = graphics.instance,
	};
	VkResult result = vmaCreateAllocator(&allocatorInfo, &graphics.allocator);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to initialize Graphics, but failed to create VmaAllocator: %i\n", result); }
}

static void CreateFramesInFlight()
{
	// create the frame resources that allow for asynchronous rendering
	for (int32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		// allocate the command buffers
		VkCommandBufferAllocateInfo allocateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandPool = graphics.commandPool,
			.commandBufferCount = 1,
		};
		vkAllocateCommandBuffers(graphics.device, &allocateInfo, &graphics.frames[i].commandBuffer);
		vkAllocateCommandBuffers(graphics.device, &allocateInfo, &graphics.frames[i].computeCommandBuffer);
		
		// create the semaphores that sync the frame resources with each other on the GPU
		VkSemaphoreCreateInfo semaphoreInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};
		vkCreateSemaphore(graphics.device, &semaphoreInfo, NULL, &graphics.frames[i].imageAvailable);
		vkCreateSemaphore(graphics.device, &semaphoreInfo, NULL, &graphics.frames[i].renderFinished);
		vkCreateSemaphore(graphics.device, &semaphoreInfo, NULL, &graphics.frames[i].computeFinished);
		
		// create the fences that sync the GPU with the CPU
		VkFenceCreateInfo fenceInfo =
		{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};
		vkCreateFence(graphics.device, &fenceInfo, NULL, &graphics.frames[i].frameReady);
		vkCreateFence(graphics.device, &fenceInfo, NULL, &graphics.frames[i].computeFence);
	}
	graphics.frameIndex = 0;
}

static void CreateSwapchainKHR(int32_t width, int32_t height)
{
	width *= ApplicationDPIScale();
	height *= ApplicationDPIScale();
	graphics.swapchain.targetExtent = (VkExtent2D){ .width = width, .height = height };
	
	// get the available capabilities of the window surface
	VkSurfaceCapabilitiesKHR availableCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(graphics.physicalDevice, graphics.surface, &availableCapabilities);
	
	// get the list of supported surface formats
	uint32_t availableFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(graphics.physicalDevice, graphics.surface, &availableFormatCount, NULL);
	VkSurfaceFormatKHR * availableFormats = malloc(availableFormatCount * sizeof(VkSurfaceFormatKHR));
	vkGetPhysicalDeviceSurfaceFormatsKHR(graphics.physicalDevice, graphics.surface, &availableFormatCount, availableFormats);
	
	// set the surface format to VK_FORMAT_R8G8B8A8_UNORM if it's supported
	VkSurfaceFormatKHR surfaceFormat = availableFormats[0];
	VkFormat targetFormat = VK_FORMAT_R8G8B8A8_UNORM;
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
	vkGetPhysicalDeviceSurfacePresentModesKHR(graphics.physicalDevice, graphics.surface, &availablePresentModeCount, NULL);
	VkPresentModeKHR * availablePresentModes = malloc(availablePresentModeCount * sizeof(VkPresentModeKHR));
	vkGetPhysicalDeviceSurfacePresentModesKHR(graphics.physicalDevice, graphics.surface, &availablePresentModeCount, availablePresentModes);
	
	// set the present mode to the target present mode if it's available (VK_PRESENT_MODE_FIFO_KHR is guarenteed to be supported)
	graphics.swapchain.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (int32_t i = 0; i < availablePresentModeCount; i++)
	{
		if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) { graphics.swapchain.presentMode = availablePresentModes[i]; }
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
	
	// setup swapchain create info
	VkSwapchainCreateInfoKHR createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = graphics.surface,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.preTransform = availableCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = graphics.swapchain.presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};
	const uint32_t queueFamilyIndices[] = { graphics.graphicsQueueIndex, graphics.presentQueueIndex };
	if (graphics.graphicsQueueIndex != graphics.presentQueueIndex)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
		createInfo.queueFamilyIndexCount = 2;
	}
	else { createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; }
	
	// Create the swapchain
	VkResult result = vkCreateSwapchainKHR(graphics.device, &createInfo, NULL, &graphics.swapchain.instance);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to create swapchain, but failed to create VkSwapchainKHR: %i\n", result); }

	graphics.swapchain.extent = extent;
	graphics.swapchain.colorFormat = surfaceFormat.format;
}

static void CreateSwapchainRenderPass()
{
	// create the single render pass that is used for all drawing operations
	VkAttachmentDescription colorAttachment =
	{
		.format = graphics.swapchain.colorFormat,
		.samples = graphics.samples,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	VkAttachmentReference colorAttachmentReference =
	{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	
	VkAttachmentDescription resolveAttachment =
	{
		.format = graphics.swapchain.colorFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};
	VkAttachmentReference resolveReference =
	{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	
	/*VkAttachmentDescription depthAttachment =
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
	};*/
	
	VkSubpassDescription subpass =
	{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentReference,
		.pResolveAttachments = &resolveReference,
	};
	
	VkAttachmentDescription attachments[] = { colorAttachment, resolveAttachment };
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
	if (graphics.samples == 1)
	{
		subpass.pResolveAttachments = NULL;
		VkAttachmentDescription attachments[] = { colorAttachment };
		renderPassInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
		renderPassInfo.pAttachments = attachments;
	}
	VkResult result = vkCreateRenderPass(graphics.device, &renderPassInfo, NULL, &graphics.swapchain.renderPass);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to initialize Graphics, but failed to create VkRenderPass: %i\n", result); }
}

static void CreateSwapchainFramebuffers()
{
	// get the list of swapchain images
	vkGetSwapchainImagesKHR(graphics.device, graphics.swapchain.instance, &graphics.swapchain.imageCount, NULL);
	graphics.swapchain.images = malloc(graphics.swapchain.imageCount * sizeof(VkImage));
	vkGetSwapchainImagesKHR(graphics.device, graphics.swapchain.instance, &graphics.swapchain.imageCount, graphics.swapchain.images);
	
	// create color imageViews
	graphics.swapchain.imageViews = malloc(graphics.swapchain.imageCount * sizeof(VkImageView));
	for (int32_t i = 0; i < graphics.swapchain.imageCount; i++)
	{
		VkImageViewCreateInfo createInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = graphics.swapchain.images[i],
			.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
			.format = graphics.swapchain.colorFormat,
			.subresourceRange =
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseArrayLayer = 0,
				.baseMipLevel = 0,
				.layerCount = 1,
				.levelCount = 1,
			},
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
		};
		VkResult result = vkCreateImageView(graphics.device, &createInfo, NULL, graphics.swapchain.imageViews + i);
		if (result != VK_SUCCESS) { printf("[Fatal] trying to create swapchain image views, but failed to create VkImageView: %i\n", result); }
	}
	
	// create framebuffers
	graphics.swapchain.framebuffers = malloc(graphics.swapchain.imageCount * sizeof(VkFramebuffer));
	for (int32_t i = 0; i < graphics.swapchain.imageCount; i++)
	{
		VkImageView attachments[] = { graphics.swapchain.msaaImageView, graphics.swapchain.imageViews[i] };
		VkFramebufferCreateInfo createInfo =
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.width = graphics.swapchain.extent.width,
			.height = graphics.swapchain.extent.height,
			.attachmentCount = sizeof(attachments) / sizeof(attachments[0]),
			.pAttachments = attachments,
			.layers = 1,
			.renderPass = graphics.swapchain.renderPass,
		};
		if (graphics.samples == 1)
		{
			VkImageView attachments[] = { graphics.swapchain.imageViews[i] };
			createInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
			createInfo.pAttachments = attachments;
		}
		VkResult result = vkCreateFramebuffer(graphics.device, &createInfo, NULL, graphics.swapchain.framebuffers + i);
		if (result != VK_SUCCESS) { printf("[Fatal] trying to create swapchain framebuffers, but failed to create VkFramebuffer: %i\n", result); }
	}
}

static void CreateSwapchainMSAA()
{
	VkImageCreateInfo imageCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.format = graphics.swapchain.colorFormat,
		.arrayLayers = 1,
		.extent = { .width = graphics.swapchain.extent.width, .height = graphics.swapchain.extent.height, 1 },
		.imageType = VK_IMAGE_TYPE_2D,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.mipLevels = 1,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = (const uint32_t []){ graphics.graphicsQueueIndex },
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.samples = graphics.samples,
	};
	VmaAllocationCreateInfo allocationInfo =
	{
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
	};
	VkResult result = vmaCreateImage(graphics.allocator, &imageCreateInfo, &allocationInfo, &graphics.swapchain.msaaImage, &graphics.swapchain.msaaImageAllocation, NULL);
	if (result != VK_SUCCESS) { printf("[Fatal] failed to create msaaImage: %i\n", result); }
	
	VkImageViewCreateInfo imageViewCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = graphics.swapchain.msaaImage,
		.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
		.format = graphics.swapchain.colorFormat,
		.subresourceRange =
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseArrayLayer = 0,
			.baseMipLevel = 0,
			.layerCount = 1,
			.levelCount = 1,
		},
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
	};
	result = vkCreateImageView(graphics.device, &imageViewCreateInfo, NULL, &graphics.swapchain.msaaImageView);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to create swapchain image views, but failed to create VkImageView: %i\n", result); }
}

static void CreateSwapchain(int32_t width, int32_t height)
{
	CreateSwapchainKHR(width, height);
	CreateSwapchainRenderPass();
	if (graphics.samples > 1) { CreateSwapchainMSAA(); }
	CreateSwapchainFramebuffers();
}

static void DestroySwapchain()
{
	vkDeviceWaitIdle(graphics.device);
	if (graphics.samples > 1)
	{
		vkDestroyImageView(graphics.device, graphics.swapchain.msaaImageView, NULL);
		vmaDestroyImage(graphics.allocator, graphics.swapchain.msaaImage, graphics.swapchain.msaaImageAllocation);
	}
	vkDestroyRenderPass(graphics.device, graphics.swapchain.renderPass, NULL);
	free(graphics.swapchain.images);
	for (int32_t i = 0; i < graphics.swapchain.imageCount; i++)
	{
		vkDestroyImageView(graphics.device, graphics.swapchain.imageViews[i], NULL);
		vkDestroyFramebuffer(graphics.device, graphics.swapchain.framebuffers[i], NULL);
	}
	free(graphics.swapchain.imageViews);
	vkDestroySwapchainKHR(graphics.device, graphics.swapchain.instance, NULL);
}

void GraphicsInitialize(int32_t width, int32_t height, int32_t msaa)
{
	printf("[Info] initializing the graphics backend...\n");
	CheckExtensionSupport();
	CreateInstance(true);
	CreateSurface();
	ChoosePhysicalDevice(msaa);
	CreateLogicalDevice();
	CreateCommandPool();
	CreateAllocator();
	CreateFramesInFlight();
	CreateSwapchain(width, height);
	printf("[Info] successfully initialized the graphics backend\n");
}

void GraphicsRecreateSwapchain(int32_t width, int32_t height)
{
	DestroySwapchain();
	CreateSwapchain(width, height);
}

void GraphicsUpdate()
{
	// advance to the next frame resource
	graphics.frameIndex = (graphics.frameIndex + 1) % FRAMES_IN_FLIGHT;
	vkWaitForFences(graphics.device, 1, &graphics.frames[graphics.frameIndex].frameReady, VK_TRUE, UINT64_MAX);
	vkResetFences(graphics.device, 1, &graphics.frames[graphics.frameIndex].frameReady);
}

void GraphicsBegin()
{
	// acquire next image (and try again if unsuccesful)
	VkResult result = vkAcquireNextImageKHR(graphics.device, graphics.swapchain.instance, UINT64_MAX, graphics.frames[graphics.frameIndex].imageAvailable, VK_NULL_HANDLE, &graphics.swapchain.imageIndex);
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) { printf("[Warning] unsuccessful aquire image: %i, trying again until successful...\n", result); }
	while (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		result = vkAcquireNextImageKHR(graphics.device, graphics.swapchain.instance, UINT64_MAX, graphics.frames[graphics.frameIndex].imageAvailable, VK_NULL_HANDLE, &graphics.swapchain.imageIndex);
		if (result == VK_SUCCESS) { printf("[Info] Succefully aquired image, continuing operations\n"); }
	}
	
	// begin recording commands
	vkResetCommandBuffer(graphics.frames[graphics.frameIndex].commandBuffer, 0);
	VkCommandBufferBeginInfo beginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	result = vkBeginCommandBuffer(graphics.frames[graphics.frameIndex].commandBuffer, &beginInfo);
	
	// begin renderpass
	VkRenderPassBeginInfo renderPassBegin =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = graphics.swapchain.renderPass,
		.framebuffer = graphics.swapchain.framebuffers[graphics.swapchain.imageIndex],
		.renderArea = (VkRect2D)
		{
			.offset = { 0, 0 },
			.extent = { graphics.swapchain.extent.width, graphics.swapchain.extent.height },
		},
		.clearValueCount = 0,
		.pClearValues = NULL,
	};
	vkCmdBeginRenderPass(graphics.frames[graphics.frameIndex].commandBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
}

void GraphicsClearColor(float r, float g, float b, float a)
{
	VkClearRect rect =
	{
		.baseArrayLayer = 0,
		.layerCount = 1,
		.rect = { .offset = { 0, 0 }, .extent = graphics.swapchain.extent, },
	};
	VkClearAttachment clear = (VkClearAttachment)
	{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.clearValue = { .color = { r, g, b, a } },
	};
	vkCmdClearAttachments(graphics.frames[graphics.frameIndex].commandBuffer, 1, &clear, 1, &rect);
}

void GraphicsBindPipeline(Pipeline pipeline)
{	
	graphics.boundPipeline = pipeline;
	vkCmdBindPipeline(graphics.frames[graphics.frameIndex].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.instance);
	VkViewport viewport =
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = graphics.swapchain.extent.width,
		.height = graphics.swapchain.extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	VkRect2D scissor =
	{
		.offset = { 0, 0 },
		.extent = graphics.swapchain.extent,
	};
	vkCmdSetViewport(graphics.frames[graphics.frameIndex].commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(graphics.frames[graphics.frameIndex].commandBuffer, 0, 1, &scissor);
}

void GraphicsRenderVertexBuffer(VertexBuffer buffer)
{
	if (graphics.boundPipeline.bindingCount > 0)
	{
		vkCmdBindDescriptorSets(graphics.frames[graphics.frameIndex].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.boundPipeline.layout, 0, 1, &graphics.boundPipeline.descriptorSets[graphics.frameIndex], 0, NULL);
	}
	
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(graphics.frames[graphics.frameIndex].commandBuffer, 0, 1, &buffer.vertexBuffer, &offset);
	vkCmdDraw(graphics.frames[graphics.frameIndex].commandBuffer, buffer.vertexCount, 1, 0, 0);
}

void GraphicsEnd()
{
	// end the render pass
	vkCmdEndRenderPass(graphics.frames[graphics.frameIndex].commandBuffer);
	
	// end command recording
	VkResult result = vkEndCommandBuffer(graphics.frames[graphics.frameIndex].commandBuffer);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to end graphics recording, but failed to record command buffer: %i\n", result); }
	
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	VkSemaphore waitSemaphores[] = { graphics.frames[graphics.frameIndex].imageAvailable };
	
	// submit the command buffer to the graphics queue
	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = sizeof(waitSemaphores) / sizeof(waitSemaphores[0]),
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &graphics.frames[graphics.frameIndex].commandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &graphics.frames[graphics.frameIndex].renderFinished,
	};
	result = vkQueueSubmit(graphics.graphicsQueue, 1, &submitInfo, graphics.frames[graphics.frameIndex].frameReady);
	if (result != VK_SUCCESS) { printf("[Fatal] trying to send graphics commands, but failed to submit queue: %i\n", result); }
	
	// submit the swapchain to the present queue
	VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &graphics.frames[graphics.frameIndex].renderFinished,
		.swapchainCount = 1,
		.pSwapchains = &graphics.swapchain.instance,
		.pImageIndices = &graphics.swapchain.imageIndex,
	};
	vkQueuePresentKHR(graphics.presentQueue, &presentInfo);
}

void GraphicsShutdown()
{
	DestroySwapchain();
	for (int32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyFence(graphics.device, graphics.frames[i].frameReady, NULL);
		vkDestroyFence(graphics.device, graphics.frames[i].computeFence, NULL);
		vkDestroySemaphore(graphics.device, graphics.frames[i].renderFinished, NULL);
		vkDestroySemaphore(graphics.device, graphics.frames[i].imageAvailable, NULL);
		vkDestroySemaphore(graphics.device, graphics.frames[i].computeFinished, NULL);
		vkFreeCommandBuffers(graphics.device, graphics.commandPool, 1, &graphics.frames[i].commandBuffer);
		vkFreeCommandBuffers(graphics.device, graphics.commandPool, 1, &graphics.frames[i].computeCommandBuffer);
	}
	vkDestroyCommandPool(graphics.device, graphics.commandPool, NULL);
	vkDestroyDevice(graphics.device, NULL);
	vkDestroySurfaceKHR(graphics.instance, graphics.surface, NULL);
	vkDestroyInstance(graphics.instance, NULL);
}
