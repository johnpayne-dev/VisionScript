#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>
#include "Backend.h"

#ifdef __APPLE__
	#include <vulkan/vulkan_macos.h>
	static const char * requiredExtensionNames[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_MVK_MACOS_SURFACE_EXTENSION_NAME };
	static uint32_t requiredExtensionCount = sizeof(requiredExtensionNames) / sizeof(requiredExtensionNames[0]);
#endif

static VkInstance instance;

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
		if (!extensionSupported) { printf("Fatal: trying to initialize Vulkan, but a required extension is not supported: %s\n", requiredExtensionNames[i]); }
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
	if (useValidations && !validationsSupported) { printf("Warning: trying to use validation layers but they are not supported.\n"); }
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
	if (result != VK_SUCCESS) { printf("Fatal: trying to initialize Vulkan, but failed to create VkInstance: %i\n", result); }
}

void InitializeGraphicsBackend()
{
	CheckExtensionSupport();
	CreateInstance(true);
}
