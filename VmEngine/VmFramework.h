#pragma once
#include "Texture.h"
#include "FBXModel.h"

typedef struct {
	VkLayerProperties properties;
	std::vector<VkExtensionProperties> extensions;
} layer_properties;

typedef struct _swap_chain_buffers {
	VkImage image;
	VkImageView view;
} swap_chain_buffer;

typedef struct {
	VkFormat format;
	VkImage image;
	VkDeviceMemory mem;
	VkImageView view;
} depth;

typedef struct {
	VkBuffer buf;
	VkDeviceMemory mem;
	VkDescriptorBufferInfo buffer_info;
} uniform_data;

class VmFramework
{
public:
	VmFramework();
	~VmFramework();

	void VmInitialize(HINSTANCE hInstance, HWND hWnd, const int nWindowWidth, const int nWindowHeight);
	void DestroyVulkan();
	void Render();

private:
	/* Amount of time, in nanoseconds, to wait for a command buffer to complete */
	enum { FENCE_TIMEOUT = 100000000 };

	const uint32_t NUM_DESCRIPTOR_SETS = 1;
	
	VkResult InitLayerProperties();
	void InitExtensionNames();
	void InitApplication();
	VkResult InitEnumerateDevice(uint32_t physicalDeviceCount = 1);
	void CreateSurface();
	void CreateDepthBuffer();
	VkResult InitDevice();
	void InitCommandPool();
	void InitCommandBuffer();
	void InitDeviceQueue();
	void InitSwapChain(VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

	bool memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
	void InitUniformBuffer();
	void InitBoneUniformBuffer();
	void InitDescriptorSetAndPipelineLayout(bool use_texture);
	void InitDescriptorPool(bool use_texture); 
	void InitDescriptorSet(bool use_texture);
	void InitRenderPass(bool include_depth);
	void InitShaders();
	void InitFrameBuffer(bool include_depth);
	void InitVertexBuffer(bool use_texture);
	void InitIndexBuffer();
	
	void InitPipelineCache();
	void InitPipeline(bool include_depth, bool include_vi = true);
	
	void Init_Viewports();
	void Init_Scissors();

	void UpdateDataBuffer();

	void CreateSemaphoreAndFence();
	
	HWND m_hWnd;
	HINSTANCE m_hInstance;

	VkInstance m_vkInstance;
	std::vector<VkQueueFamilyProperties> m_vvkQueueFamilyProperties;
	std::vector<VkPhysicalDevice> m_vvkPhysicalDevices;
	VkDevice m_vkDevice;
	std::vector<const char*> m_vvkDeviceExtensionNames;
	std::vector<const char*> m_vvkInstanceExtensionNames;
	std::vector<layer_properties> m_vkInstanceLayerProperties;
	VkSurfaceKHR m_vkSurface;
	VkFormat m_vkFormat;

	VkSwapchainKHR m_vkSwapchain;
	std::vector<swap_chain_buffer> m_vvkSwapchainBuffers;
	uint32_t m_nSwapchainImageCount;
	VkCommandBuffer m_vkCommandBuffer;
	VkCommandPool m_vkCommandPool;

	uint32_t m_nQueueFamilyPropertyCount;
	uint32_t m_nGraphicQueueFamilyIndex;
	uint32_t m_nGraphicQueueFamilyCount;
	uint32_t m_nPresentQueueFamilyIndex;

	int m_nWindowWidth;
	int m_nWindowHeight;

	VkPhysicalDeviceMemoryProperties m_vkPhysicalDeviceMemoryProperites;
	VkPhysicalDeviceProperties m_vkPhysicalDeviceProperties;

	depth m_Depth;
	uniform_data m_UniformData;
	uniform_data m_BoneUniformData;

	std::vector<VkDescriptorSetLayout> m_vDesc_Layout;
	std::vector<VkDescriptorSet> m_vDesc_Set;

	VkPipelineLayout m_vkPipeline_Layout;
	VkPipelineCache m_vkPipeline_Cache;
	VkDescriptorPool m_Desc_Pool;	

	VkRenderPass m_Render_Pass;

	VkPipelineShaderStageCreateInfo m_ShaderStages[2];
	
	char* read_spv(const char* filename, size_t* pSize);

	VkFramebuffer* m_pFrameBuffers;

	VkQueue m_vkGraphicsQueue;
	VkQueue m_vkPresentQueue;

	struct {
		VkBuffer buf;
		VkDeviceMemory mem;
		VkDescriptorBufferInfo buffer_info;
	} vertex_buffer;

	struct {
		VkBuffer buf;
		VkDeviceMemory mem;
		VkDescriptorBufferInfo buffer_info;
	} index_buffer;

	VkVertexInputBindingDescription m_VIBinding;
	VkVertexInputAttributeDescription m_VIAttribs[4];
	uint32_t m_nCurrent_Buffer;

	VkPipeline m_vkPipeline;
	VkViewport m_vkViewport;
	VkRect2D m_vkScissor;
	
	VkFence m_vkDrawFence[2];
	VkSemaphore m_vkImageAcquiredSemaphores[2];
	VkSemaphore m_vkDrawCompleteSemaphores[2];

	glm::mat4 m_mtxVP;
	glm::mat4 m_mtxModel;
	float m_fRotateAngle;

	Texture* m_pCubeTexture;
	texture_object m_Texture;

	FBXModel* m_pFbxModel;

	// ------------------------------
	float m_fCameraPosX;
	float m_fCameraPosY;
	float m_fCameraPosZ;
	public:
	void CameraMove(WPARAM wParam);
};

