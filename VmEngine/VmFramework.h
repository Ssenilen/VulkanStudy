#pragma once
#include "Texture.h"
#include "FBXModel.h"

#define ERR_EXIT(err_msg, err_class)							\
    do {														\
        MessageBox(nullptr, L##err_msg, L##err_class, MB_OK);	\
        exit(1);												\
    } while (0)

// Allow a maximum of two outstanding presentation operations.
#define FRAME_LAG 2

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
	
	void InitVulkan();
	void InitVKInstance();
	VkResult InitEnumerateDevice(); // GPU 정보를 얻어온다.
	void InitVKSwapChain();
	VkResult InitDevice();

	uint32_t enabled_extension_count;
	uint32_t enabled_layer_count;
	std::vector<const char*> m_vvkExtensionNames;
	VkPhysicalDevice m_vkPhysicalDevice;
	VkPhysicalDeviceProperties m_vkPhysicalDeviceProperties;
	uint32_t m_nQueueFamilyCount; /// PropertyCount -> Count
	std::unique_ptr<VkQueueFamilyProperties[]> m_pvkQueueFamilyProperties;
	VkSurfaceKHR m_vkSurface;
	uint32_t m_nGraphicQueueFamilyIndex;
	uint32_t m_nPresentQueueFamilyIndex;
	bool m_bSeparatePresentQueue;
	VkQueue m_vkGraphicsQueue;
	VkQueue m_vkPresentQueue;
	VkFormat m_vkFormat;
	VkColorSpaceKHR m_vkColorSpace;
	VkFence m_vkDrawFence[2];
	VkSemaphore m_vkImageAcquiredSemaphores[2];
	VkSemaphore m_vkDrawCompleteSemaphores[2];
	VkSemaphore m_vkLargeOwnershipSemaphores[2];
	uint32_t m_nCurrFrame;
	uint32_t m_nFrameIndex;
	VkPhysicalDeviceMemoryProperties m_vkPhysicalDeviceMemoryProperites;
	///

	void CreateDepthBuffer();
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

	HWND m_hWnd;
	HINSTANCE m_hInstance;

	VkInstance m_vkInstance;
	std::vector<VkQueueFamilyProperties> m_vvkQueueFamilyProperties;

	VkDevice m_vkDevice;
	std::vector<const char*> m_vvkDeviceExtensionNames;
	std::vector<layer_properties> m_vkInstanceLayerProperties;

	VkSwapchainKHR m_vkSwapchain;
	std::vector<swap_chain_buffer> m_vvkSwapchainBuffers;
	uint32_t m_nSwapchainImageCount;
	VkCommandBuffer m_vkCommandBuffer;
	VkCommandPool m_vkCommandPool;


	int m_nWindowWidth;
	int m_nWindowHeight;


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

