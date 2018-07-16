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

class VmBuffer;
class VmGameObject;
class VmFramework
{
public:
	VmFramework();
	~VmFramework();

	void VmInitialize(HINSTANCE hInstance, HWND hWnd, TCHAR* pWindowTitle, const int nWindowWidth, const int nWindowHeight);
	void DestroyVulkan();
	void Tick();
	void Render();

private:
	/* Amount of time, in nanoseconds, to wait for a command buffer to complete */
	enum { FENCE_TIMEOUT = 100000000 };

	void InitVulkan();
	void InitVKInstance();
	VkResult InitEnumerateDevice(); // GPU 정보를 얻어온다.
	void InitVKSwapChain();
	VkResult InitDevice();
	void CreateCommandPoolAndCommandBuffer();
	void CreateSwapChainBuffer(VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	void CreateDepthBuffer();

	void CalculateFrameStats();

	VkDeviceManager m_vkDeviceManager;
	MatrixManager m_MatrixManager;

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
	///

	bool memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
	void InitUniformBuffer();
	void InitBoneUniformBuffer();
	void SetupDescriptorSetLayout(bool use_texture);
	void InitDescriptorPool(bool use_texture); 
	/// FRAMEWORK에서 드러낼 부분
	//void InitDescriptorSet(bool use_texture);
	/// FRAMEWORK에서 드러낼 부분
	void InitRenderPass(bool include_depth);
	void InitShaders();
	void InitFrameBuffer(bool include_depth);
	void InitVertexBuffer(bool use_texture);
	void InitIndexBuffer();
	void InitInstanceBuffer();
	void InitPipeline(bool include_depth, bool include_vi = true);
	
	// Render 관련
	void SetViewports();
	void SetScissors();
	void Present();

	HWND m_hWnd;
	HINSTANCE m_hInstance;
	TCHAR* m_pWindowTitle;

	VkInstance m_vkInstance;
	std::vector<VkQueueFamilyProperties> m_vvkQueueFamilyProperties;

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

	//VkDescriptorSet m_vkDescriptorSet;

	VkPipelineLayout m_vkPipeline_Layout;

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

	struct {
		VkBuffer buf;
		VkDeviceMemory mem;
		VkDescriptorBufferInfo buffer_info;
	} instance_buffer;

	VkVertexInputBindingDescription m_VIBinding[2];
	VkVertexInputAttributeDescription m_VIAttribs[8];
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
	FBXModel* m_pFbxModel2;
	FBXModel* m_pFbxModel3;

	// ------------------------------
	float m_fCameraPosX;
	float m_fCameraPosY;
	float m_fCameraPosZ;
	public:
	void CameraMove(WPARAM wParam);

	// ------- 임시로 Framework를 Scene으로 간주하고 쓴다 ---
	std::vector<VmGameObject*> m_pGameObject;
};

