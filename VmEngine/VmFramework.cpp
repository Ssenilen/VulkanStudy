#include "stdafx.h"
#include "VmFramework.h"

#include "cube_data.h"

#define NUM_VIEWPORTS 1
#define NUM_SCISSORS NUM_VIEWPORTS
#define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT

VmFramework::VmFramework()
	: m_hWnd(0),
	m_hInstance(0),
	m_vkInstance(0),
	m_vkDevice(0),
	m_vkSurface(0),
	m_vkSwapchain(0),
	m_vkCommandBuffer(0),
	m_vkCommandPool(0),
	m_nSwapchainImageCount(0),
	m_nQueueFamilyCount(0),
	m_nGraphicQueueFamilyIndex(0),
	m_nPresentQueueFamilyIndex(0),
	m_nWindowWidth(0),
	m_nWindowHeight(0),
	m_fRotateAngle(0.00f),
	m_fCameraPosX(0.0f),
	m_fCameraPosY(50.0f),
	m_fCameraPosZ(375.0f)
{
}


VmFramework::~VmFramework()
{
}

void VmFramework::VmInitialize(HINSTANCE hInstance, HWND hWnd, const int nWindowWidth, const int nWindowHeight)
{
	VkResult result;
	const bool depthPresent = true;

	m_hInstance = hInstance;
	m_hWnd = hWnd;
	m_nWindowWidth = nWindowWidth;
	m_nWindowHeight = nWindowHeight;

	InitVulkan();
	InitVKSwapChain();
	///
	InitCommandPool();
	InitCommandBuffer();
	InitDeviceQueue();
	InitSwapChain();
	CreateDepthBuffer();
	InitUniformBuffer();

	m_pFbxModel = new FBXModel("cg_dance2.fbx");
	InitBoneUniformBuffer();

	m_pFbxModel->GetPositionData();

	InitDescriptorSetAndPipelineLayout(true);
	InitRenderPass(depthPresent);
	InitShaders();
	InitFrameBuffer(depthPresent);

	
	InitVertexBuffer(true);
	InitIndexBuffer();
	InitDescriptorPool(true);

	m_pCubeTexture = new Texture(m_vkDevice, m_vkCommandBuffer, m_vkPhysicalDeviceMemoryProperites, "castle_guard.png");
	m_Texture = m_pCubeTexture->GetTextureObject();

	InitDescriptorSet(true);
	InitPipelineCache();
	InitPipeline(depthPresent);
}

void VmFramework::InitVKInstance()
{
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = nullptr;
	app_info.pApplicationName = "GS_VULKAN_STUDY";
	app_info.applicationVersion = 0;
	app_info.pEngineName = "GS_VULKAN_STUDY";
	app_info.engineVersion = 0;
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo inst_info = {};
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pNext = nullptr;
	inst_info.flags = 0;
	inst_info.pApplicationInfo = &app_info;
	inst_info.enabledLayerCount = 0;
	inst_info.ppEnabledLayerNames = nullptr;
	inst_info.enabledExtensionCount = m_vvkExtensionNames.size();
	inst_info.ppEnabledExtensionNames =
		inst_info.enabledExtensionCount ? m_vvkExtensionNames.data() : nullptr;

	VkResult result = vkCreateInstance(&inst_info, NULL, &m_vkInstance);
	assert(result == VK_SUCCESS);
}

VkResult VmFramework::InitEnumerateDevice()
{
	uint32_t gpu_count = 0;
	VkResult result = vkEnumeratePhysicalDevices(m_vkInstance, &gpu_count, nullptr);
	assert(result == VK_SUCCESS);

//	m_vvkPhysicalDevices.resize(physicalDeviceCount);

	if (gpu_count > 0)
	{
		std::unique_ptr<VkPhysicalDevice[]> physical_devices(new VkPhysicalDevice[gpu_count]);
		result = vkEnumeratePhysicalDevices(m_vkInstance, &gpu_count, physical_devices.get());
		assert(result == VK_SUCCESS);
		m_vkPhysicalDevice = physical_devices[0];
	}
	else
	{
		ERR_EXIT(
			"vkEnumeratePhysicalDevices reported zero accessible devices.\n\n"
			"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkEnumeratePhysicalDevices Failure");
	}

	/* Look for device extensions */
	uint32_t device_extension_count = 0;
	VkBool32 swapchainExtFound = VK_FALSE;
	m_vvkExtensionNames.clear();

	result = vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, nullptr, &device_extension_count, nullptr);
	assert(result == VK_SUCCESS);

	if (device_extension_count > 0)
	{
		std::unique_ptr<VkExtensionProperties[]> device_extensions(new VkExtensionProperties[device_extension_count]);
		result = vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, nullptr, &device_extension_count, device_extensions.get());
		assert(result == VK_SUCCESS);

		for (uint32_t i = 0; i < device_extension_count; ++i)
		{
			if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, device_extensions[i].extensionName)) {
				swapchainExtFound = 1;
				m_vvkExtensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
			}
			assert(m_vvkExtensionNames.size() < 64);
		}
	}  
	if (!swapchainExtFound) {
		ERR_EXIT("vkEnumerateDeviceExtensionProperties failed to find the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
			" extension.\n\n"
			"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkCreateInstance Failure");
	}
	vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &m_vkPhysicalDeviceProperties);
	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &m_nQueueFamilyCount, nullptr);
	assert(m_nQueueFamilyCount >= 1);

	m_pvkQueueFamilyProperties.reset(new VkQueueFamilyProperties[m_nQueueFamilyCount]);
	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &m_nQueueFamilyCount, m_pvkQueueFamilyProperties.get());

	// Query fine-grained feature support for this device.
	//  If app has specific feature requirements it should check supported
	//  features based on this query
	VkPhysicalDeviceFeatures physDevFeatures;
	vkGetPhysicalDeviceFeatures(m_vkPhysicalDevice, &physDevFeatures);

	return result;
}

void VmFramework::InitVKSwapChain()
{
	VkResult result;

	// Surface 생성
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.pNext = nullptr;
	surfaceCreateInfo.hinstance = m_hInstance;
	surfaceCreateInfo.hwnd = m_hWnd;
	result = vkCreateWin32SurfaceKHR(m_vkInstance, &surfaceCreateInfo, nullptr, &m_vkSurface);
	assert(result == VK_SUCCESS);

	std::unique_ptr<VkBool32[]> supportsPresent(new VkBool32[m_nQueueFamilyCount]);
	for (uint32_t i = 0; i < m_nQueueFamilyCount; ++i)
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(m_vkPhysicalDevice, i, m_vkSurface, &supportsPresent[i]);
	}

	uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
	uint32_t presentQueueFamilyIndex = UINT32_MAX;
	for (uint32_t i = 0; i < m_nQueueFamilyCount; ++i)
	{
		if (m_pvkQueueFamilyProperties[i].queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT)
		{
			if (graphicsQueueFamilyIndex == UINT32_MAX)
			{
				graphicsQueueFamilyIndex = i;
			}

			if (supportsPresent[i] == VK_TRUE)
			{
				graphicsQueueFamilyIndex = i;
				presentQueueFamilyIndex = i;
				break;
			}
		}
	}

	if (presentQueueFamilyIndex == UINT32_MAX)
	{
		// graphic, present에 해당하는 큐를 못 찾았다면 별도의 present Queue를 찾는다.
		for (uint32_t i = 0; i < m_nQueueFamilyCount; ++i) {
			if (supportsPresent[i] == VK_TRUE) {
				presentQueueFamilyIndex = i;
				break;
			}
		}
	}

	if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX) 
	{
		ERR_EXIT("Could not find both graphics and present queues\n", "Swapchain Initialization Failure");
	}

	m_nGraphicQueueFamilyIndex = graphicsQueueFamilyIndex;
	m_nPresentQueueFamilyIndex = presentQueueFamilyIndex;
	m_bSeparatePresentQueue = (m_nGraphicQueueFamilyIndex != m_nPresentQueueFamilyIndex);

	InitDevice();

	vkGetDeviceQueue(m_vkDevice, m_nGraphicQueueFamilyIndex, 0, &m_vkGraphicsQueue);
	if (!m_bSeparatePresentQueue)
	{
		m_vkPresentQueue = m_vkGraphicsQueue;
	}
	else
	{
		vkGetDeviceQueue(m_vkDevice, m_nPresentQueueFamilyIndex, 0, &m_vkPresentQueue);
	}

	// Get the list of VkFormat's that are supported:
	uint32_t formatCount;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurface, &formatCount, nullptr);
	assert(result == VK_SUCCESS);

	std::unique_ptr<VkSurfaceFormatKHR[]> surfFormats(new VkSurfaceFormatKHR[formatCount]);
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurface, &formatCount, surfFormats.get());
	assert(result == VK_SUCCESS);

	// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
	// the surface has no preferred format.  Otherwise, at least one
	// supported format will be returned.
	if (formatCount == 1 && surfFormats[0].format == VkFormat::VK_FORMAT_UNDEFINED) {
		m_vkFormat = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
	}
	else {
		assert(formatCount >= 1);
		m_vkFormat = surfFormats[0].format;
	}
	m_vkColorSpace = surfFormats[0].colorSpace;
	m_nCurrFrame = 0;

	// Create semaphores to synchronize acquiring presentable buffers before
	// rendering and waiting for drawing to be complete before presenting
	VkSemaphoreCreateInfo semaphoreCreateInfo = VkSemaphoreCreateInfo();
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	// Create fences that we can use to throttle if we get too far
	// ahead of the image presents
	VkFenceCreateInfo fence_ci = VkFenceCreateInfo();
	fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_ci.pNext = nullptr;
	fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < FRAME_LAG; i++) {
		result = vkCreateFence(m_vkDevice, &fence_ci, nullptr, &m_vkDrawFence[i]);
		assert(result == VK_SUCCESS);

		result = vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr, &m_vkImageAcquiredSemaphores[i]);
		assert(result == VK_SUCCESS);

		result = vkCreateSemaphore( m_vkDevice, &semaphoreCreateInfo, nullptr, &m_vkDrawCompleteSemaphores[i]);
		assert(result == VK_SUCCESS);

		if (m_bSeparatePresentQueue) {
			result = vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr, &m_vkLargeOwnershipSemaphores[i]);
			assert(result == VK_SUCCESS);
		}
	}
	m_nFrameIndex = 0;

	// Get Memory information and properties
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &m_vkPhysicalDeviceMemoryProperites);
}

VkResult VmFramework::InitDevice()
{
	VkResult result;
	VkDeviceQueueCreateInfo deviceQueueCreateInfo[2] = {};
	float queue_priorities[1] = { 0.0 };
	
	deviceQueueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo[0].pNext = nullptr;
	deviceQueueCreateInfo[0].queueCount = 1;
	deviceQueueCreateInfo[0].pQueuePriorities = queue_priorities;
	deviceQueueCreateInfo[0].queueFamilyIndex = m_nGraphicQueueFamilyIndex;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo[0];
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
	deviceCreateInfo.enabledExtensionCount = m_vvkExtensionNames.size();
	deviceCreateInfo.ppEnabledExtensionNames =
		deviceCreateInfo.enabledExtensionCount ? m_vvkExtensionNames.data() : nullptr;
	deviceCreateInfo.pEnabledFeatures = nullptr;

	if (m_bSeparatePresentQueue)
	{
		deviceQueueCreateInfo[1].queueFamilyIndex = m_nPresentQueueFamilyIndex;
		deviceQueueCreateInfo[1].queueCount = 1;
		deviceQueueCreateInfo[1].pQueuePriorities = queue_priorities;
		deviceCreateInfo.queueCreateInfoCount = 2;
	}

	result = vkCreateDevice(m_vkPhysicalDevice, &deviceCreateInfo, NULL, &m_vkDevice);
	assert(result == VK_SUCCESS);
	return result;
}

void VmFramework::InitCommandPool()
{
	VkResult result;

	VkCommandPoolCreateInfo commandPoolInfo = {};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;
	commandPoolInfo.queueFamilyIndex = m_nGraphicQueueFamilyIndex;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	
	result = vkCreateCommandPool(m_vkDevice, &commandPoolInfo, nullptr, &m_vkCommandPool);
	assert(result == VK_SUCCESS);
}

void VmFramework::InitCommandBuffer()
{
	// InitSwapChain과 InitCommandPool이 선행되어야 한다.
	VkResult result;

	VkCommandBufferAllocateInfo commandBufferInfo = {};
	commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferInfo.pNext = NULL;
	commandBufferInfo.commandPool = m_vkCommandPool;
	commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferInfo.commandBufferCount = 1;

	result = vkAllocateCommandBuffers(m_vkDevice, &commandBufferInfo, &m_vkCommandBuffer);
	assert(result == VK_SUCCESS);

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = NULL;
	commandBufferBeginInfo.flags = 0;
	commandBufferBeginInfo.pInheritanceInfo = NULL;

	result = vkBeginCommandBuffer(m_vkCommandBuffer, &commandBufferBeginInfo);
	assert(result == VK_SUCCESS);
}

void VmFramework::InitDeviceQueue()
{
	vkGetDeviceQueue(m_vkDevice, m_nGraphicQueueFamilyIndex, 0, &m_vkGraphicsQueue);
	if (m_nGraphicQueueFamilyIndex == m_nPresentQueueFamilyIndex)
	{
		m_vkPresentQueue = m_vkGraphicsQueue;
	}
	else
	{
		vkGetDeviceQueue(m_vkDevice, m_nPresentQueueFamilyIndex, 0, &m_vkPresentQueue);
	}
}

void VmFramework::InitSwapChain(VkImageUsageFlags usageFlags)
{
	VkResult result;
	VkSurfaceCapabilitiesKHR surfCapabilities;

	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysicalDevice, m_vkSurface, &surfCapabilities);
	assert(result == VK_SUCCESS);

	uint32_t presentModeCount;
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice, m_vkSurface, &presentModeCount, nullptr);
	assert(result == VK_SUCCESS);

	VkPresentModeKHR* presentModes = (VkPresentModeKHR*)malloc(presentModeCount * sizeof(VkPresentModeKHR));
	assert(presentModes);
	
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice, m_vkSurface, &presentModeCount, presentModes);
	assert(result == VK_SUCCESS);

	VkExtent2D swapchainExtent;
	// width와 height가 모두 0xFFFFFFFF이거나, 둘 다 0xFFFFFFFF이 아니어야 한다.
	if (surfCapabilities.currentExtent.width == 0xFFFFFFFF)
	{
		// Surface의 크기가 정의되지 않은 경우 WindowSize로 설정
		swapchainExtent.width = m_nWindowWidth;
		swapchainExtent.height = m_nWindowWidth;
		if (swapchainExtent.width < surfCapabilities.minImageExtent.width) {
			swapchainExtent.width = surfCapabilities.minImageExtent.width;
		}
		else if (swapchainExtent.width >
			surfCapabilities.maxImageExtent.width) {
			swapchainExtent.width = surfCapabilities.maxImageExtent.width;
		}

		if (swapchainExtent.height < surfCapabilities.minImageExtent.height) {
			swapchainExtent.height = surfCapabilities.minImageExtent.height;
		}
		else if (swapchainExtent.height >
			surfCapabilities.maxImageExtent.height) {
			swapchainExtent.height = surfCapabilities.maxImageExtent.height;
		}
	}
	else {
		// Surface의 크기가 정해져 있다면, Swapchain의 크기와 일치해야 한다.
		swapchainExtent = surfCapabilities.currentExtent;
	}
	// The FIFO present mode is guaranteed by the spec to be supported
	// Also note that current Android driver only supports FIFO
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	// Swapchain에서 사용할 vkImage의 수를 결정하는 부분
	uint32_t desiredNumberOfSwapChainImages = surfCapabilities.minImageCount;

	VkSurfaceTransformFlagBitsKHR preTransform;
	if (surfCapabilities.supportedTransforms &
		VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else {
		preTransform = surfCapabilities.currentTransform;
	}

	VkSwapchainCreateInfoKHR swapchain_ci = {};
	swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_ci.pNext = nullptr;
	swapchain_ci.surface = m_vkSurface;
	swapchain_ci.minImageCount = desiredNumberOfSwapChainImages;
	swapchain_ci.imageFormat = m_vkFormat;
	swapchain_ci.imageExtent.width = swapchainExtent.width;
	swapchain_ci.imageExtent.height = swapchainExtent.height;
	swapchain_ci.preTransform = preTransform;
	swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_ci.imageArrayLayers = 1;
	swapchain_ci.presentMode = swapchainPresentMode;
	swapchain_ci.oldSwapchain = VK_NULL_HANDLE;
	swapchain_ci.clipped = true;
	swapchain_ci.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapchain_ci.imageUsage = usageFlags;
	swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_ci.queueFamilyIndexCount = 0;
	swapchain_ci.pQueueFamilyIndices = NULL;
	uint32_t queueFamilyIndices[2] = {
		(uint32_t)m_nGraphicQueueFamilyIndex,
		(uint32_t)m_nPresentQueueFamilyIndex };
	if (m_nGraphicQueueFamilyIndex != m_nPresentQueueFamilyIndex) {
		// If the graphics and present queues are from different queue families,
		// we either have to explicitly transfer ownership of images between the
		// queues, or we have to create the swapchain with imageSharingMode
		// as VK_SHARING_MODE_CONCURRENT
		swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_ci.queueFamilyIndexCount = 2;
		swapchain_ci.pQueueFamilyIndices = queueFamilyIndices;
	}
	result = vkCreateSwapchainKHR(m_vkDevice, &swapchain_ci, nullptr, &m_vkSwapchain);
	assert(result == VK_SUCCESS);

	result = vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &m_nSwapchainImageCount, nullptr);
	assert(result == VK_SUCCESS);

	VkImage *swapchainImages = (VkImage *)malloc(m_nSwapchainImageCount * sizeof(VkImage));
	assert(swapchainImages);
	result = vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &m_nSwapchainImageCount, swapchainImages);
	assert(result == VK_SUCCESS);

	for (uint32_t i = 0; i < m_nSwapchainImageCount; i++) {
		swap_chain_buffer sc_buffer;

		VkImageViewCreateInfo color_image_view = {};
		color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		color_image_view.pNext = NULL;
		color_image_view.format = m_vkFormat;
		color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
		color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
		color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
		color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
		color_image_view.subresourceRange.aspectMask =
			VK_IMAGE_ASPECT_COLOR_BIT;
		color_image_view.subresourceRange.baseMipLevel = 0;
		color_image_view.subresourceRange.levelCount = 1;
		color_image_view.subresourceRange.baseArrayLayer = 0;
		color_image_view.subresourceRange.layerCount = 1;
		color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		color_image_view.flags = 0;

		sc_buffer.image = swapchainImages[i];

		color_image_view.image = sc_buffer.image;

		result = vkCreateImageView(m_vkDevice, &color_image_view, nullptr, &sc_buffer.view);
		m_vvkSwapchainBuffers.push_back(sc_buffer);
		assert(result == VK_SUCCESS);
	}
	free(swapchainImages);
	m_nCurrent_Buffer = 0;

	if (NULL != presentModes) {
		free(presentModes);
	}
}

void VmFramework::DestroyVulkan()
{
	for (uint32_t i = 0; i < m_nSwapchainImageCount; ++i)
	{
		vkDestroySemaphore(m_vkDevice, m_vkImageAcquiredSemaphores[i], nullptr);
		vkDestroyFence(m_vkDevice, m_vkDrawFence[i], nullptr);
	}
	vkDestroyPipeline(m_vkDevice, m_vkPipeline, nullptr);
	vkDestroyBuffer(m_vkDevice, vertex_buffer.buf, nullptr);
	vkFreeMemory(m_vkDevice, vertex_buffer.mem, nullptr);

	for (uint32_t i = 0; i < m_nSwapchainImageCount; i++) {
		vkDestroyFramebuffer(m_vkDevice, m_pFrameBuffers[i], NULL);
	}
	delete[] m_pFrameBuffers;

	vkDestroyShaderModule(m_vkDevice, m_ShaderStages[0].module, nullptr);
	vkDestroyShaderModule(m_vkDevice, m_ShaderStages[1].module, nullptr);

	vkDestroyRenderPass(m_vkDevice, m_Render_Pass, nullptr);
	vkDestroyDescriptorPool(m_vkDevice, m_Desc_Pool, nullptr);

	// destroy_uniform_buffer
	vkDestroyBuffer(m_vkDevice, m_UniformData.buf, nullptr);
	vkFreeMemory(m_vkDevice, m_UniformData.mem, nullptr);

	// destroy_descriptor_and_pipeline_layouts
	for (uint32_t i = 0; i < NUM_DESCRIPTOR_SETS; i++)
		vkDestroyDescriptorSetLayout(m_vkDevice, m_vDesc_Layout[i], nullptr);
	vkDestroyPipelineLayout(m_vkDevice, m_vkPipeline_Layout, nullptr);

	for (uint32_t i = 0; i < m_nSwapchainImageCount; i++)
	{
		vkDestroyImageView(m_vkDevice, m_vvkSwapchainBuffers[i].view, nullptr);
	}
	vkDestroySwapchainKHR(m_vkDevice, m_vkSwapchain, nullptr);

	VkCommandBuffer cmd_bufs[1] = { m_vkCommandBuffer };
	vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, cmd_bufs);
	vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);

	vkDeviceWaitIdle(m_vkDevice);
	vkDestroyDevice(m_vkDevice, nullptr);
	vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
	DestroyWindow(m_hWnd);

	vkDestroyInstance(m_vkInstance, nullptr);
}

void VmFramework::InitVulkan()
{
	uint32_t instance_extension_count = 0;
	uint32_t instance_layer_count = 0;
	uint32_t validation_layer_count = 0; // 아직까지의 예제에서 사용하지 않았다.
	enabled_layer_count = 0;

	// Cube 예시에서 인스턴스 레이어 수를 감지하지 않은 채로 사용했다.
	// 인스턴스 레이어의 개념을 다시 한 번 복습하자.
	// VkResult result = vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr);
	VkBool32 surfaceExtFound = VK_FALSE;
	VkBool32 platformSurfaceExtFound = VK_FALSE;

	VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr);
	assert(result == VkResult::VK_SUCCESS);

	if (instance_extension_count > 0)
	{
		std::unique_ptr<VkExtensionProperties[]> instance_extensions(new VkExtensionProperties[instance_extension_count]);
		result = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, instance_extensions.get());
		assert(result == VkResult::VK_SUCCESS);

		for (uint32_t i = 0; i < instance_extension_count; ++i)
		{
			if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName)) {
				surfaceExtFound = 1;
				m_vvkExtensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
			}
			if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName)) {
				platformSurfaceExtFound = 1;
				m_vvkExtensionNames.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
			}
			assert(m_vvkExtensionNames.size() < 64);
		}
	}
	if (!surfaceExtFound) {
		ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_SURFACE_EXTENSION_NAME
			" extension.\n\n"
			"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkCreateInstance Failure");
	}

	if (!platformSurfaceExtFound) {
		ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_WIN32_SURFACE_EXTENSION_NAME
			" extension.\n\n"
			"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkCreateInstance Failure");
	}

	InitVKInstance();
	InitEnumerateDevice();
}

void VmFramework::CreateDepthBuffer()
{	
	VkResult result;
	
	VkImageCreateInfo image_info = {};
	if (m_Depth.format == VK_FORMAT_UNDEFINED)
		m_Depth.format = VK_FORMAT_D32_SFLOAT;
		//m_Depth.format = VK_FORMAT_D16_UNORM;

	const VkFormat depth_format = m_Depth.format;

	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(m_vkPhysicalDevice, m_Depth.format, &props);
	if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) 
	{
		image_info.tiling = VK_IMAGE_TILING_LINEAR;
	}
	else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) 
	{
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	}
	else
	{
		/* Try other depth formats? */
		std::cout << "depth_format " << depth_format << " Unsupported.\n";
		exit(-1);
	}

	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.pNext = nullptr;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = depth_format;
	image_info.extent.width = m_nWindowWidth;
	image_info.extent.height = m_nWindowHeight;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	// 샘플의 수는 이미지 생성, 렌더패스 생성 및 파이프라인 생성 개수와 같아야 한다.
	image_info.samples = NUM_SAMPLES;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.queueFamilyIndexCount = 0;
	image_info.pQueueFamilyIndices = nullptr;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image_info.flags = 0;
	
	/* Create image */
	result = vkCreateImage(m_vkDevice, &image_info, nullptr, &m_Depth.image);
	assert(result == VK_SUCCESS);

	VkMemoryAllocateInfo mem_alloc = {};
	mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc.pNext = nullptr;
	mem_alloc.allocationSize = 0;
	mem_alloc.memoryTypeIndex = 0;

	VkImageViewCreateInfo view_info = {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.pNext = nullptr;
	view_info.image = m_Depth.image;
	view_info.format = depth_format;
	view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;// VK_COMPONENT_SWIZZLE_R;
	view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;//VK_COMPONENT_SWIZZLE_G;
	view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;//VK_COMPONENT_SWIZZLE_B;
	view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;//VK_COMPONENT_SWIZZLE_A;
	view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.flags = 0;

	if (depth_format == VK_FORMAT_D16_UNORM_S8_UINT ||
		depth_format == VK_FORMAT_D24_UNORM_S8_UINT ||
		depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
		view_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	VkMemoryRequirements mem_reqs;

	vkGetImageMemoryRequirements(m_vkDevice, m_Depth.image, &mem_reqs);

	mem_alloc.allocationSize = mem_reqs.size;
	/* Use the memory properties to determine the type of memory required */
	bool pass = memory_type_from_properties(mem_reqs.memoryTypeBits,
		0, /* No requirements */
		&mem_alloc.memoryTypeIndex);
	assert(pass);

	/* Allocate memory */
	result = vkAllocateMemory(m_vkDevice, &mem_alloc, nullptr, &m_Depth.mem);
	assert(result == VK_SUCCESS);

	/* Bind memory */
	result = vkBindImageMemory(m_vkDevice, m_Depth.image, m_Depth.mem, 0);
	assert(result == VK_SUCCESS);

	/* Create image view */
	view_info.image = m_Depth.image;
	result = vkCreateImageView(m_vkDevice, &view_info, NULL, &m_Depth.view);
	assert(result == VK_SUCCESS);
}

bool VmFramework::memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex)
{
	for (uint32_t i = 0; i < m_vkPhysicalDeviceMemoryProperites.memoryTypeCount; i++)
	{
		if ((typeBits & 1) == 1)
		{
			// 사용할 수 있는 타입인지, 사용자 속성과 일치하는지
			if ((m_vkPhysicalDeviceMemoryProperites.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
			{
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	return false;
}

void VmFramework::InitUniformBuffer()
{
	VkResult result;
	
	// 7. Uniform Buffer
	// Uniform Buffer는 셰이더가 상수 매개 변수 데이터를 읽을 수 있도록 하는 읽기 전용의 버퍼이다.
	// 다른 그래픽 API에선 수행하지 않아도 되는, Vulkan 프로그램에서 수행하는 단계이다.
	// GLES에서 셰이더로 보내지는 Uniform Buffer의 내용을 설정하기 위한 API를 호출하고, 메모리를 할당하여 채워 넣는다.

	// 셰이더에 Model-View-Projection 행렬을 전달하고, 셰이더가 각 정점을 변환할 수 있게 한다.
	float fov = glm::radians(45.0f);
	//if (m_nWindowWidth > m_nWindowHeight)
	//{
	//	fov *= static_cast<float>(m_nWindowHeight) / static_cast<float>(m_nWindowWidth);
	//}

	//mat4x4 mtxProjection, mtxView, mtxMVP2, mtxMVP;
	//mat4x4_perspective(mtxProjection, fov, static_cast<float>(m_nWindowHeight) / static_cast<float>(m_nWindowWidth), 0.1f, 100.0f);	

	//vec3 eye = { 0.0f, 150.0f, 250.0f };	// World Space에서의 카메라 위치
	//vec3 origin = { 0.0f, 0.0f, -400.0f };	// 카메라가 바라보는 좌표
	//vec3 up = { 0.0f, 1.0f, 0.0f };		// 업벡터를 거꾸로 설정해주어야 한다. (아래에서 -1 곱해줌)
	//vec3 eye = {0.0f, 3.0f, 5.0f };	// World Space에서의 카메라 위치
	//vec3 origin = { 0.0f, 0.0f, 0.0f };	// 카메라가 바라보는 좌표
	//vec3 up = { 0.0f, 1.0f, 0.0f };		// 업벡터를 거꾸로 설정해주어야 한다. (아래에서 -1 곱해줌)

	//mat4x4_look_at(mtxView, eye, origin, up);
	//mat4x4_identity(m_mtxModel);
	//mtxProjection[1][1] *= (-1);  //Flip projection matrix from GL to Vulkan orientation.

	//mat4x4_mul(m_mtxVP, mtxProjection, mtxView);
	//mat4x4_mul(mtxMVP, m_mtxVP, m_mtxModel);
	//
	//mat4x4 clip{ 1.0f, 0.0f, 0.0f, 0.0f,
	//			0.0f, -1.0f, 0.0f, 0.0f,
	//			0.0f, 0.0f, 0.5f, 0.0f,
	//			0.0f, 0.0f, 0.5f, 1.0f };

	//mat4x4_mul(mtxMVP2, clip, mtxMVP);

	//float mvp[4][4];
	//memcpy(mvp, mtxMVP, sizeof(mtxMVP));

	glm::mat4 mtxProjection = glm::perspective(fov, static_cast<float>(m_nWindowWidth) / static_cast<float>(m_nWindowHeight), 0.1f, 100.0f);
	//glm::mat4 mtxView = glm::lookAt(glm::vec3(0.0f, 180.0f, 250.0f),
	//								glm::vec3(0.0f, 0.0f, -400.0f),
	//								glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 mtxView = glm::lookAt(glm::vec3(25.0f, 0.0f, 0.0f), 
									glm::vec3(0.0f, 0.0f, 0.0f),
									glm::vec3(0.0f, 1.0f, 0.0f));

	//mtxProjection[1][1] *= -1;

	glm::mat4 m_mtxModel = glm::mat4(1.0f);
	// Vulkan clip space has inverted Y and half Z.
	glm::mat4 mtxClip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
								  0.0f, -1.0f, 0.0f, 0.0f,
						  		  0.0f, 0.0f, 0.5f, 0.5f,
								  0.0f, 0.0f, 0.0f, 1.0f);

	m_mtxVP = mtxClip * mtxProjection * mtxView;
	glm::mat4 mtxMVP = m_mtxVP * m_mtxModel;

	// Uniform Buffer 객체 생성
	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.pNext = nullptr;
	buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buf_info.size = sizeof(mtxMVP);
	buf_info.queueFamilyIndexCount = 0;
	buf_info.pQueueFamilyIndices = nullptr;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_info.flags = 0;
	result = vkCreateBuffer(m_vkDevice, &buf_info, nullptr, &m_UniformData.buf);
	assert(result == VK_SUCCESS);

	// Uniform Buffer 메모리 할당
	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(m_vkDevice, m_UniformData.buf, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.memoryTypeIndex = 0;

	alloc_info.allocationSize = mem_reqs.size;
	bool bPass = memory_type_from_properties(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &alloc_info.memoryTypeIndex);
	assert(bPass && "No mappable, coherent memory");

	result = vkAllocateMemory(m_vkDevice, &alloc_info, nullptr, &(m_UniformData.mem));
	assert(result == VK_SUCCESS);

	// Uniform Buffer Memory 매핑 및 설정
	// Depth Buffer는 메모리 내용을 초기화 할 필요가 없지만, 
	// Uniform Buffer는 쉐이더가 읽을 데이터(MVP 행렬)로 채워 넣어야 한다.
	// * 메모리에 대한 CPU 액세스를 얻으려면 맵핑 해야 한다.
	uint8_t *pData;
	result = vkMapMemory(m_vkDevice, m_UniformData.mem, 0, mem_reqs.size, 0, (void**)&pData);
	assert(result == VK_SUCCESS);

	memcpy(pData, &mtxMVP, sizeof(mtxMVP));
	vkUnmapMemory(m_vkDevice, m_UniformData.mem);

	result = vkBindBufferMemory(m_vkDevice, m_UniformData.buf, m_UniformData.mem, 0);
	assert(result == VK_SUCCESS);

	m_UniformData.buffer_info.buffer = m_UniformData.buf;
	m_UniformData.buffer_info.offset = 0;
	m_UniformData.buffer_info.range = sizeof(mtxMVP);
	// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT는 CPU가 메모리에 액세스할 수 있도록 메모리가 맵핑되어야 함을 알린다.
	// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT는 메모리 캐시를 flush할 필요 없이 CPU에 의한 메모리 쓰기가 Device에 표시되도록 요청.
	// 이런 설정을 해 놓으면 vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRangesGPU에 데이터 표시를 확인할 필요가 없어진다.
}

void VmFramework::InitBoneUniformBuffer()
{
	VkResult result;	
	glm::mat4 mtxSize;

	// Uniform Buffer 객체 생성
	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.pNext = nullptr;
	buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buf_info.size = sizeof(mtxSize) * 96;
	buf_info.queueFamilyIndexCount = 0;
	buf_info.pQueueFamilyIndices = nullptr;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_info.flags = 0;
	result = vkCreateBuffer(m_vkDevice, &buf_info, nullptr, &m_BoneUniformData.buf);
	assert(result == VK_SUCCESS);

	// Uniform Buffer 메모리 할당
	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(m_vkDevice, m_BoneUniformData.buf, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.memoryTypeIndex = 0;

	alloc_info.allocationSize = mem_reqs.size;
	bool bPass = memory_type_from_properties(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &alloc_info.memoryTypeIndex);
	assert(bPass && "No mappable, coherent memory");

	result = vkAllocateMemory(m_vkDevice, &alloc_info, nullptr, &(m_BoneUniformData.mem));
	assert(result == VK_SUCCESS);

	uint8_t *pData;
	result = vkMapMemory(m_vkDevice, m_BoneUniformData.mem, 0, mem_reqs.size, 0, (void**)&pData);
	assert(result == VK_SUCCESS);

	m_pFbxModel->GetBoneUniformData(pData);
	vkUnmapMemory(m_vkDevice, m_BoneUniformData.mem);

	result = vkBindBufferMemory(m_vkDevice, m_BoneUniformData.buf, m_BoneUniformData.mem, 0);
	assert(result == VK_SUCCESS);

	m_BoneUniformData.buffer_info.buffer = m_BoneUniformData.buf;
	m_BoneUniformData.buffer_info.offset = 0;
	m_BoneUniformData.buffer_info.range = sizeof(mtxSize) * 96;
}

void VmFramework::InitDescriptorSetAndPipelineLayout(bool use_texture)
{
	VkDescriptorSetLayoutBinding layout_bindings[3];
	layout_bindings[0].binding = 0;
	layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layout_bindings[0].descriptorCount = 1;
	layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layout_bindings[0].pImmutableSamplers = nullptr;
	layout_bindings[1].binding = 1;
	layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layout_bindings[1].descriptorCount = 1;
	layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layout_bindings[1].pImmutableSamplers = nullptr;

	if (use_texture) {
		layout_bindings[2].binding = 2;
		layout_bindings[2].descriptorType =
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layout_bindings[2].descriptorCount = 1;
		layout_bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layout_bindings[2].pImmutableSamplers = nullptr;
	}

	// 이후, Layout Binding을 하고 이를 이용해 Descriptor Set Layout을 생성한다.
	VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
	descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_layout.pNext = nullptr;
	descriptor_layout.bindingCount = use_texture ? 3 : 2;
	descriptor_layout.pBindings = layout_bindings;

	VkResult result;
	
	m_vDesc_Layout.resize(NUM_DESCRIPTOR_SETS);
	result = vkCreateDescriptorSetLayout(m_vkDevice, &descriptor_layout, nullptr, m_vDesc_Layout.data());
	
	assert(result == VK_SUCCESS);
	
	// descriptor layiout을 이용해 pipeline layout을 생성한다.
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
	pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.pNext = nullptr;
	pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pPipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	pPipelineLayoutCreateInfo.setLayoutCount = NUM_DESCRIPTOR_SETS;
	pPipelineLayoutCreateInfo.pSetLayouts =m_vDesc_Layout.data();


	result = vkCreatePipelineLayout(m_vkDevice, &pPipelineLayoutCreateInfo, nullptr, &m_vkPipeline_Layout);
	assert(result == VK_SUCCESS);
}

void VmFramework::InitDescriptorPool(bool use_texture)
{	
	// Uniform Buffer가 Init되고, DerscriptorAndPipelineLayouts 함수가 호출된 이후에 시행되어야 한다.
	VkResult result;

	VkDescriptorPoolSize type_count[3];
	type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	type_count[0].descriptorCount = 1;
	type_count[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	type_count[1].descriptorCount = 1;
	if (use_texture) 
	{
		type_count[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		type_count[2].descriptorCount = 1;
	}

	VkDescriptorPoolCreateInfo descriptor_pool = {};
	descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool.pNext = nullptr;
	descriptor_pool.maxSets = 1;
	descriptor_pool.poolSizeCount = use_texture ? 3 : 2;
	descriptor_pool.pPoolSizes = type_count;

	result = vkCreateDescriptorPool(m_vkDevice, &descriptor_pool, nullptr, &m_Desc_Pool);
	assert(result == VK_SUCCESS);
}

void VmFramework::InitDescriptorSet(bool use_texture)
{
	VkResult result;
	
	// 앞서 호출한 InitDescriptorPool을 이용해 DiscriptorPool을 생성하고 나면,
	// Pool에서 Descriptor를 할당할 수 있게 된다.
	VkDescriptorSetAllocateInfo alloc_info[1];
	alloc_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info[0].pNext = nullptr;
	alloc_info[0].descriptorPool = m_Desc_Pool;
	alloc_info[0].descriptorSetCount = NUM_DESCRIPTOR_SETS;
	alloc_info[0].pSetLayouts = m_vDesc_Layout.data();
	
	m_vDesc_Set.resize(NUM_DESCRIPTOR_SETS);

	result = vkAllocateDescriptorSets(m_vkDevice, alloc_info, m_vDesc_Set.data());
	assert(result == VK_SUCCESS);

	// 디스크립터 셋 업데이트
	VkWriteDescriptorSet writes[3];

	writes[0] = {};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].pNext = nullptr;
	writes[0].dstSet = m_vDesc_Set[0];
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &m_UniformData.buffer_info;
	writes[0].dstArrayElement = 0;
	writes[0].dstBinding = 0;


	writes[1] = {};
	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].pNext = nullptr;
	writes[1].dstSet = m_vDesc_Set[0];
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[1].pBufferInfo = &m_BoneUniformData.buffer_info;
	writes[1].dstArrayElement = 0;
	writes[1].dstBinding = 1;

	if (use_texture)
	{
		VkDescriptorImageInfo tex_descs[TEXTURE_COUNT];
		memset(&tex_descs, 0, sizeof(tex_descs));
		for (unsigned int i = 0; i < TEXTURE_COUNT; i++)
		{
			tex_descs[i].sampler = m_Texture.sampler;
			tex_descs[i].imageView = m_Texture.view;
			tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		}

		writes[2] = {};
		writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[2].dstSet = m_vDesc_Set[0];
		writes[2].dstBinding = 2;
		writes[2].descriptorCount = 1;
		writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[2].pImageInfo = tex_descs;
		writes[2].dstArrayElement = 0;
	}

	// Cube.c 2227 Line
	vkUpdateDescriptorSets(m_vkDevice, use_texture ? 3 : 2, writes, 0, nullptr);
 }

void VmFramework::InitRenderPass(bool include_depth)
{
	VkResult result;

	// Render Target과 Depth Buffer에 대한 첨부 파일이 필요하다.
	VkAttachmentDescription attachments[2];
	attachments[0].format = m_vkFormat;
	attachments[0].samples = NUM_SAMPLES;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachments[0].flags = 0;

	if (include_depth)
	{
		attachments[1].format = m_Depth.format;
		attachments[1].samples = NUM_SAMPLES;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].flags = 0;
	}

	VkAttachmentReference color_reference = {};
	color_reference.attachment = 0;
	color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_reference = {};
	depth_reference.attachment = 1;
	depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_reference;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = include_depth ? &depth_reference : nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	VkRenderPassCreateInfo rp_info = {};
	rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rp_info.pNext = NULL;
	rp_info.attachmentCount = include_depth ? 2 : 1;
	rp_info.pAttachments = attachments;
	rp_info.subpassCount = 1;
	rp_info.pSubpasses = &subpass;
	rp_info.dependencyCount = 0;
	rp_info.pDependencies = NULL;

	result = vkCreateRenderPass(m_vkDevice, &rp_info, nullptr, &m_Render_Pass);
	assert(result == VK_SUCCESS);



	/* Vertex Buffer 부분 생성하며 중복 코드가 있어 주석 처리
	VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo;
	imageAcquiredSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	imageAcquiredSemaphoreCreateInfo.pNext = nullptr;
	imageAcquiredSemaphoreCreateInfo.flags = 0;

	result = vkCreateSemaphore(m_vkDevice, &imageAcquiredSemaphoreCreateInfo, nullptr, &m_ImageAcquiredSemaphore);
	assert(result == VK_SUCCESS);

	// 렌더 패스는 두 가지의 attachment가 존재한다. (색상 & 깊이)
	
	// 레이아웃 설정을 위해 Swapchain 이미지를 획득한다.
	result = vkAcquireNextImageKHR(m_vkDevice, m_vkSwapchain, UINT64_MAX,
		m_ImageAcquiredSemaphore, VK_NULL_HANDLE, &m_nCurrent_Buffer);
	assert(result >= 0);
	*/

	// The initial layout for the color and depth attachments will be
	// LAYOUT_UNDEFINED because at the start of the renderpass, we don't
	// care about their contents. At the start of the subpass, the color
	// attachment's layout will be transitioned to LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	// and the depth stencil attachment's layout will be transitioned to
	// LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.  At the end of the renderpass,
	// the color attachment's layout will be transitioned to
	// LAYOUT_PRESENT_SRC_KHR to be ready to present.  This is all done as part
	// of the renderpass, no barriers are necessary.
}

void VmFramework::InitShaders()
{
	VkResult result;

	VkShaderModuleCreateInfo moduleCreateInfo;

	// Vertex Shader
	m_ShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_ShaderStages[0].pNext = nullptr;
	m_ShaderStages[0].pSpecializationInfo = nullptr;
	m_ShaderStages[0].flags = 0;
	m_ShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	m_ShaderStages[0].pName = "main";

	size_t size = 0;
	void* vertShaderCode = read_spv("default_vert.spv", &size);

	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.pNext = nullptr;
	moduleCreateInfo.flags = 0;
	moduleCreateInfo.codeSize = size;// *sizeof(uint32_t);
	moduleCreateInfo.pCode = (uint32_t*)vertShaderCode;
	result = vkCreateShaderModule(m_vkDevice, &moduleCreateInfo, nullptr, &m_ShaderStages[0].module);
	assert(result == VK_SUCCESS);
	free(vertShaderCode);

	// Fragment Shader
	m_ShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_ShaderStages[1].pNext = nullptr;
	m_ShaderStages[1].pSpecializationInfo = nullptr;
	m_ShaderStages[1].flags = 0;
	m_ShaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_ShaderStages[1].pName = "main";

	size = 0;
	void* fragShaderCode = read_spv("default_frag.spv", &size);

	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.pNext = nullptr;
	moduleCreateInfo.flags = 0;
	moduleCreateInfo.codeSize = size;// *sizeof(uint32_t);
	moduleCreateInfo.pCode = (uint32_t*)fragShaderCode;
	result = vkCreateShaderModule(m_vkDevice, &moduleCreateInfo, nullptr, &m_ShaderStages[1].module);
	assert(result == VK_SUCCESS);
	free(fragShaderCode);
}

void VmFramework::InitFrameBuffer(bool include_depth)
{
	VkResult result;

	VkImageView attachments[2];
	attachments[1] = m_Depth.view;

	VkFramebufferCreateInfo fb_info = {};
	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.pNext = nullptr;
	fb_info.renderPass = m_Render_Pass;
	fb_info.attachmentCount = include_depth ? 2 : 1;
	fb_info.pAttachments = attachments;
	fb_info.width = m_nWindowWidth;
	fb_info.height = m_nWindowHeight;
	fb_info.layers = 1;

	
	m_pFrameBuffers = new VkFramebuffer[m_nSwapchainImageCount];
	for (uint32_t i = 0; i < m_nSwapchainImageCount; i++)
	{
		attachments[0] = m_vvkSwapchainBuffers[i].view;
		result = vkCreateFramebuffer(m_vkDevice, &fb_info, nullptr, &m_pFrameBuffers[i]);
		assert(result == VK_SUCCESS);
	}
}

void VmFramework::InitVertexBuffer(bool use_texture)
{
	VkResult result;
	// 13. 버텍스 버퍼 생성
	// 1) Create Buffer

	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.pNext = nullptr;
	buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	buf_info.size = m_pFbxModel->GetVertexDataSize() * sizeof(float) * 16;
	buf_info.queueFamilyIndexCount = 0;
	buf_info.pQueueFamilyIndices = nullptr;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_info.flags = 0;
	result = vkCreateBuffer(m_vkDevice, &buf_info, NULL, &vertex_buffer.buf);
	assert(result == VK_SUCCESS);

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(m_vkDevice, vertex_buffer.buf, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.memoryTypeIndex = 0;

	alloc_info.allocationSize = mem_reqs.size;

	bool pass = memory_type_from_properties(mem_reqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&alloc_info.memoryTypeIndex);
	assert(pass && "No mappable, coherent memory"); // 여기까지

	result = vkAllocateMemory(m_vkDevice, &alloc_info, nullptr, &vertex_buffer.mem);
	assert(result == VK_SUCCESS);

	vertex_buffer.buffer_info.range = mem_reqs.size;
	vertex_buffer.buffer_info.offset = 0;

	uint8_t* pData;
	result = vkMapMemory(m_vkDevice, vertex_buffer.mem, 0, mem_reqs.size, 0, (void**)&pData);
	assert(result == VK_SUCCESS);

	//memcpy(pData, g_vb_IndexCube, sizeof(g_vb_IndexCube));
	m_pFbxModel->GetVBDataAndMemcpy(pData);
	vkUnmapMemory(m_vkDevice, vertex_buffer.mem);

	result = vkBindBufferMemory(m_vkDevice, vertex_buffer.buf, vertex_buffer.mem, 0);
	assert(result == VK_SUCCESS);

	// 여기서는 필요하지 않지만, 파이프라인을 만들 때에 필요한 정보들.
	m_VIBinding.binding = 0;
	m_VIBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	m_VIBinding.stride = sizeof(float) * 16;
	// 여기서 stride는 한 개의 정점 크기를 의미한다.

	// Vertex
	m_VIAttribs[0].binding = 0;
	m_VIAttribs[0].location = 0;
	m_VIAttribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_VIAttribs[0].offset = 0;
	// TextureUV or Color
	m_VIAttribs[1].binding = 0;
	m_VIAttribs[1].location = 1;
	m_VIAttribs[1].format = use_texture ? VK_FORMAT_R32G32_SFLOAT : VK_FORMAT_R32G32B32A32_SFLOAT;
	m_VIAttribs[1].offset = m_VIAttribs[0].offset + 16;
	// BoneIndices
	m_VIAttribs[2].binding = 0;
	m_VIAttribs[2].location = 2;
	m_VIAttribs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_VIAttribs[2].offset = m_VIAttribs[1].offset + 16;
	// BoneWeights
	m_VIAttribs[3].binding = 0;
	m_VIAttribs[3].location = 3;
	m_VIAttribs[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_VIAttribs[3].offset = m_VIAttribs[2].offset + 16;
}

void VmFramework::InitIndexBuffer()
{
	VkResult result;

	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.pNext = nullptr;
	buf_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	buf_info.size = m_pFbxModel->GetIndicesSize();
	buf_info.queueFamilyIndexCount = 0;
	buf_info.pQueueFamilyIndices = nullptr;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_info.flags = 0;
	result = vkCreateBuffer(m_vkDevice, &buf_info, nullptr, &index_buffer.buf);
	assert(result == VK_SUCCESS);

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(m_vkDevice, index_buffer.buf, &mem_reqs);
	
	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.memoryTypeIndex = 0;

	alloc_info.allocationSize = mem_reqs.size;

	bool pass = memory_type_from_properties(mem_reqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&alloc_info.memoryTypeIndex);
	assert(pass && "No mappable, coherent memory");

	result = vkAllocateMemory(m_vkDevice, &alloc_info, nullptr, &index_buffer.mem);
	assert(result == VK_SUCCESS);

	index_buffer.buffer_info.range = mem_reqs.size;
	index_buffer.buffer_info.offset = 0;

	uint16_t* pData;
	result = vkMapMemory(m_vkDevice, index_buffer.mem, 0, mem_reqs.size, 0, (void**)&pData);
	assert(result == VK_SUCCESS);

	//memcpy(pData, g_cubeIndices, sizeof(g_cubeIndices));
	m_pFbxModel->GetIndexDataAndMemcpy(pData);

	vkUnmapMemory(m_vkDevice, index_buffer.mem);

	result = vkBindBufferMemory(m_vkDevice, index_buffer.buf, index_buffer.mem, 0);
	assert(result == VK_SUCCESS);
}

void VmFramework::InitPipelineCache()
{
	VkResult result;

	VkPipelineCacheCreateInfo pipelineCache;
	pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCache.pNext = nullptr;
	pipelineCache.initialDataSize = 0;
	pipelineCache.pInitialData = nullptr;
	pipelineCache.flags = 0;
	result = vkCreatePipelineCache(m_vkDevice, &pipelineCache, nullptr, &m_vkPipeline_Cache);
	assert(result == VK_SUCCESS);
}

void VmFramework::InitPipeline(bool include_depth, bool include_vi)
{
	VkResult result;

	// Dynamic Pipeline State는 Command Buffer의 실행 중 명령에 의해 변경될 수 있는 상태를 말한다.
	// 여기선 모두 비활성화 한 상태로 시작한다.
	VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pNext = NULL;
	dynamicState.pDynamicStates = dynamicStateEnables;
	dynamicState.dynamicStateCount = 0;

	VkPipelineVertexInputStateCreateInfo vi;
	memset(&vi, 0, sizeof(vi));
	vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	if (include_vi) {
		vi.pNext = NULL;
		vi.flags = 0;
		vi.vertexBindingDescriptionCount = 1;
		vi.pVertexBindingDescriptions = &m_VIBinding;
		vi.vertexAttributeDescriptionCount = 4;
		vi.pVertexAttributeDescriptions = m_VIAttribs;
	}

	VkPipelineInputAssemblyStateCreateInfo ia;
	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.pNext = NULL;
	ia.flags = 0;
	ia.primitiveRestartEnable = VK_FALSE;
	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo rs{};
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.pNext = nullptr;
	rs.flags = 0;
	rs.polygonMode = VK_POLYGON_MODE_FILL;
	rs.cullMode = VK_CULL_MODE_BACK_BIT;
	rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;// VK_FRONT_FACE_CLOCKWISE;
	rs.depthClampEnable = include_depth;
	rs.rasterizerDiscardEnable = VK_FALSE;
	rs.depthBiasEnable = VK_FALSE;
	rs.depthBiasConstantFactor = 0;
	rs.depthBiasClamp = 0;
	rs.depthBiasSlopeFactor = 0;
	rs.lineWidth = 1.0f;

	VkPipelineColorBlendStateCreateInfo cb;
	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	cb.flags = 0;
	cb.pNext = nullptr;
	VkPipelineColorBlendAttachmentState att_state[1];
	att_state[0].colorWriteMask = 0xf;
	att_state[0].blendEnable = VK_FALSE;
	att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
	att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
	att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	cb.attachmentCount = 1;
	cb.pAttachments = att_state;
	cb.logicOpEnable = VK_FALSE;
	cb.logicOp = VK_LOGIC_OP_NO_OP;
	cb.blendConstants[0] = 1.0f;
	cb.blendConstants[1] = 1.0f;
	cb.blendConstants[2] = 1.0f;
	cb.blendConstants[3] = 1.0f;

	VkPipelineViewportStateCreateInfo vp = {};
	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp.pNext = nullptr;
	vp.flags = 0;
	vp.viewportCount = NUM_VIEWPORTS;
	dynamicStateEnables[dynamicState.dynamicStateCount++] =
		VK_DYNAMIC_STATE_VIEWPORT;
	vp.scissorCount = NUM_SCISSORS;
	dynamicStateEnables[dynamicState.dynamicStateCount++] =
		VK_DYNAMIC_STATE_SCISSOR;
	vp.pScissors = nullptr;
	vp.pViewports = nullptr;

	VkPipelineDepthStencilStateCreateInfo ds;
	ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	ds.pNext = nullptr;
	ds.flags = 0;
	ds.depthTestEnable = include_depth;
	ds.depthWriteEnable = include_depth;
	ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	ds.depthBoundsTestEnable = VK_FALSE;
	ds.stencilTestEnable = VK_FALSE;
	ds.back.failOp = VK_STENCIL_OP_KEEP;
	ds.back.passOp = VK_STENCIL_OP_KEEP;
	ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
	ds.back.compareMask = 0;
	ds.back.reference = 0;
	ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
	ds.back.writeMask = 0;
	ds.minDepthBounds = 0.0f;
	ds.maxDepthBounds = 1.0f;
	ds.front = ds.back;

	VkPipelineMultisampleStateCreateInfo ms;
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.pNext = nullptr;
	ms.flags = 0;
	ms.pSampleMask = nullptr;
	ms.rasterizationSamples = NUM_SAMPLES;
	ms.sampleShadingEnable = VK_FALSE;
	ms.alphaToCoverageEnable = VK_FALSE;
	ms.alphaToOneEnable = VK_FALSE;
	ms.minSampleShading = 0.0;

	VkGraphicsPipelineCreateInfo pipeline;
	pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline.pNext = nullptr;
	pipeline.layout = m_vkPipeline_Layout;
	pipeline.basePipelineHandle = VK_NULL_HANDLE;
	pipeline.basePipelineIndex = 0;
	pipeline.flags = 0;
	pipeline.pVertexInputState = &vi;
	pipeline.pInputAssemblyState = &ia;
	pipeline.pRasterizationState = &rs;
	pipeline.pColorBlendState = &cb;
	pipeline.pTessellationState = NULL;
	pipeline.pMultisampleState = &ms;
	pipeline.pDynamicState = &dynamicState;
	pipeline.pViewportState = &vp;
	pipeline.pDepthStencilState = &ds;
	pipeline.pStages = m_ShaderStages;
	pipeline.stageCount = 2;
	pipeline.renderPass = m_Render_Pass;
	pipeline.subpass = 0;

	result = vkCreateGraphicsPipelines(m_vkDevice, m_vkPipeline_Cache, 1, &pipeline, nullptr, &m_vkPipeline);
	assert(result == VK_SUCCESS); /// ShaderStage ㅡㅡ
}

void VmFramework::Init_Viewports()
{
	m_vkViewport.width = (float)m_nWindowWidth;
	m_vkViewport.height = (float)m_nWindowHeight;
	m_vkViewport.minDepth = (float)0.0f;
	m_vkViewport.maxDepth = (float)1.0f;
	m_vkViewport.x = 0;
	m_vkViewport.y = 0;
	vkCmdSetViewport(m_vkCommandBuffer, 0, NUM_VIEWPORTS, &m_vkViewport);
}

void VmFramework::Init_Scissors()
{
	m_vkScissor.extent.width = m_nWindowWidth;
	m_vkScissor.extent.height = m_nWindowHeight;
	m_vkScissor.offset.x = 0;
	m_vkScissor.offset.y = 0;
	vkCmdSetScissor(m_vkCommandBuffer, 0, NUM_SCISSORS, &m_vkScissor);
}

void VmFramework::UpdateDataBuffer()
{
	VkResult result;
	uint8_t *pData;
	//mat4x4 mtxRotation, mtxMVP, mtxModel, mtxMVP2;
	//mat4x4_dup(mtxModel, m_mtxModel);
	//mat4x4_rotate(m_mtxModel, mtxModel, 0.0f, 1.0f, 0.0f, (float)degreesToRadians(1.0f));
	//mat4x4_mul(m_mtxModel, m_mtxModel, mtxRotation);
	
	vkWaitForFences(m_vkDevice, 1, &m_vkDrawFence[m_nCurrent_Buffer], VK_TRUE, UINT64_MAX);
	vkResetFences(m_vkDevice, 1, &m_vkDrawFence[m_nCurrent_Buffer]);

	result = vkAcquireNextImageKHR(m_vkDevice, m_vkSwapchain, UINT64_MAX, m_vkImageAcquiredSemaphores[m_nCurrent_Buffer], m_vkDrawFence[m_nCurrent_Buffer], &m_nCurrent_Buffer);



	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		// demo->swapchain is out of date (e.g. the window was resized) and
		// must be recreated:
		// demo_resize(demo);
		// TODO: resize 처리 만들어야 함.
	}
	else if (result == VK_SUBOPTIMAL_KHR) {
		// demo->swapchain is not as optimal as it could be, but the platform's
		// presentation engine will still present the image correctly.
	}
	else {
		assert(result == VK_SUCCESS);
	}

	
	//m_mtxModel[3][0] += 0.001f;

	//mat4x4_mul(mtxMVP, m_mtxVP, m_mtxModel);

	//mat4x4 clip{ 1.0f, 0.0f, 0.0f, 0.0f,
	//			0.0f, -1.0f, 0.0f, 0.0f,
	//			0.0f, 0.0f, 0.5f, 0.0f,
	//			0.0f, 0.0f, 0.5f, 1.0f };

	//mat4x4_mul(mtxMVP2, clip, mtxMVP);

	/// ---임시임시임시임시--- 나중에 카메라기능 따로 뺍시다 ---임시임시임시임시---
	/// ---임시임시임시임시--- 나중에 카메라기능 따로 뺍시다 ---임시임시임시임시---
	/// ---임시임시임시임시--- 나중에 카메라기능 따로 뺍시다 ---임시임시임시임시---
	glm::mat4 mtxView = glm::lookAt(glm::vec3(m_fCameraPosX, m_fCameraPosY, m_fCameraPosZ),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	float fov = glm::radians(45.0f);
	glm::mat4 mtxProjection = glm::perspective(fov, static_cast<float>(m_nWindowWidth) / static_cast<float>(m_nWindowHeight), 0.1f, 100.0f);

	glm::mat4 m_mtxModel = glm::mat4(1.0f);
	// Vulkan clip space has inverted Y and half Z.
	glm::mat4 mtxClip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.5f,
		0.0f, 0.0f, 0.0f, 1.0f);

	m_mtxVP = mtxClip * mtxProjection * mtxView;
	glm::mat4 mtxMVP = m_mtxVP * m_mtxModel;
	/// ---임시임시임시임시--- 나중에 카메라기능 따로 뺍시다 ---임시임시임시임시---
	/// ---임시임시임시임시--- 나중에 카메라기능 따로 뺍시다 ---임시임시임시임시---
	/// ---임시임시임시임시--- 나중에 카메라기능 따로 뺍시다 ---임시임시임시임시---

	//glm::mat4 mtxModel = m_mtxModel;

	//m_mtxModel = glm::rotate(m_mtxModel, m_fRotateAngle, glm::vec3(0.0f, 1.0f, 0.0f));
	//glm::mat4 mtxMVP = m_mtxVP * m_mtxModel;

	VkMemoryRequirements mem_reqs;
	
	//vkWaitForFences(m_vkDevice, 1, &m_vkDrawFence, VK_TRUE, UINT64_MAX);
	vkWaitForFences(m_vkDevice, 1, &m_vkDrawFence[m_nCurrent_Buffer], VK_TRUE, UINT64_MAX);
	
	vkGetBufferMemoryRequirements(m_vkDevice, m_UniformData.buf, &mem_reqs);
	result = vkMapMemory(m_vkDevice, m_UniformData.mem, 0, VK_WHOLE_SIZE, 0, (void**)&pData);
	assert(result == VK_SUCCESS);

	memcpy(pData, &mtxMVP, sizeof(mtxMVP));

	vkUnmapMemory(m_vkDevice, m_UniformData.mem);

	// m_BoneUniform
	vkGetBufferMemoryRequirements(m_vkDevice, m_BoneUniformData.buf, &mem_reqs);
	result = vkMapMemory(m_vkDevice, m_BoneUniformData.mem, 0, VK_WHOLE_SIZE, 0, (void**)&pData);
	assert(result == VK_SUCCESS);
	m_pFbxModel->GetBoneUniformData(pData);
	vkUnmapMemory(m_vkDevice, m_BoneUniformData.mem);


	VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info = {};
	submit_info.pNext = nullptr;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pWaitDstStageMask = &pipe_stage_flags;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &m_vkImageAcquiredSemaphores[m_nCurrent_Buffer];
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_vkCommandBuffer;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &m_vkDrawCompleteSemaphores[m_nCurrent_Buffer];

	// 실행을 위한 CommandBuffer의 Queue
	result = vkQueueSubmit(m_vkGraphicsQueue, 1, &submit_info, m_vkDrawFence[m_nCurrent_Buffer]);
	assert(result == VK_SUCCESS);

	// Image를 Window에 Present하는 부분
	VkPresentInfoKHR present;
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.pNext = nullptr;
	present.swapchainCount = 1;
	present.pSwapchains = &m_vkSwapchain;
	present.pImageIndices = &m_nCurrent_Buffer;
	present.pWaitSemaphores = nullptr;
	present.waitSemaphoreCount = 0;
	present.pResults = nullptr;

	result = vkQueuePresentKHR(m_vkPresentQueue, &present);
	// Present 전에, Command Buffer가 만들어졌는지 확인하며 대기
	//do {
	//	result = vkWaitForFences(m_vkDevice, 1, &m_vkDrawFence, VK_TRUE, FENCE_TIMEOUT);
	//} while (result == VK_TIMEOUT);
	//vkResetFences(m_vkDevice, 1, &m_vkDrawFence);
	//assert(result == VK_SUCCESS);

	m_nCurrent_Buffer++;
	m_nCurrent_Buffer %= m_nSwapchainImageCount;
}

void VmFramework::Render()
{
	VkResult result;

	VkClearValue clear_values[2];
	clear_values[0].color = { 0.2f, 0.2f, 0.2f, 0.2f };
	clear_values[1].depthStencil.depth = 1.0f;
	clear_values[1].depthStencil.stencil = 0;

	VkRenderPassBeginInfo rp_begin;
	rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_begin.pNext = nullptr;
	rp_begin.renderPass = m_Render_Pass;
	rp_begin.framebuffer = m_pFrameBuffers[m_nCurrent_Buffer]; 
	rp_begin.renderArea.offset.x = 0;
	rp_begin.renderArea.offset.y = 0;
	rp_begin.renderArea.extent.width = m_nWindowWidth;
	rp_begin.renderArea.extent.height = m_nWindowHeight;
	rp_begin.clearValueCount = 2;
	rp_begin.pClearValues = clear_values;

	VkCommandBufferBeginInfo cmd_buf_info = {};
	cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_buf_info.pNext = nullptr;
	cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	cmd_buf_info.pInheritanceInfo = nullptr;

	result = vkBeginCommandBuffer(m_vkCommandBuffer, &cmd_buf_info);
	assert(result == VK_SUCCESS);
	vkCmdBeginRenderPass(m_vkCommandBuffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(m_vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline);
	vkCmdBindDescriptorSets(m_vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline_Layout, 0, NUM_DESCRIPTOR_SETS, m_vDesc_Set.data(), 0, nullptr);

	const VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(m_vkCommandBuffer, 0, 1, &vertex_buffer.buf, offsets);
	vkCmdBindIndexBuffer(m_vkCommandBuffer, index_buffer.buf, 0, VK_INDEX_TYPE_UINT16);
	Init_Viewports();
	Init_Scissors();

	//vkCmdDraw(m_vkCommandBuffer, 12 * 3, 1, 0, 0);
	vkCmdDrawIndexed(m_vkCommandBuffer, m_pFbxModel->GetIndices(), 1, 0, 0, 0);
	vkCmdEndRenderPass(m_vkCommandBuffer);

	result = vkEndCommandBuffer(m_vkCommandBuffer);
	assert(result == VK_SUCCESS);

	const VkCommandBuffer cmd_bufs[] = { m_vkCommandBuffer };

	UpdateDataBuffer();

	Sleep(1);
}

char* VmFramework::read_spv(const char* filename, size_t* pSize)
{
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		return nullptr;
	}

	fseek(fp, 0L, SEEK_END);
	long int size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);

	void *shader_code = malloc(size);
	size_t retval = fread(shader_code, size, 1, fp);
	assert(retval == 1);

	*pSize = size;

	fclose(fp);

	return (char *)shader_code;
}

void VmFramework::CameraMove(WPARAM wParam)
{
	switch (wParam)
	{
	case 'a':
	case 'A':
		m_fCameraPosX += 5.0f;
		break;
	case 'd':
	case 'D':
		m_fCameraPosX -= 5.0f;
		break;
	case 'w':
	case 'W':
		m_fCameraPosZ += 5.0f;
		break;
	case 's':
	case 'S':
		m_fCameraPosZ -= 5.0f;
		break;
	case 'q':
	case 'Q':
		m_fCameraPosY += 5.0f;
		break;
	case 'e':
	case 'E':
		m_fCameraPosY -= 5.0f;
		break;
	case 'z':
	case 'Z':
		m_fCameraPosX = 0.0f;
		m_fCameraPosY = 0.0f;
		m_fCameraPosZ = 0.0f;
		break;
	case 'c':
	case 'C':
		m_pFbxModel->AdvanceFrame();
		break;
	}
	
	std::cout << "Camera Pos: " << m_fCameraPosX << " " << m_fCameraPosY << " " << m_fCameraPosZ << std::endl;
}
