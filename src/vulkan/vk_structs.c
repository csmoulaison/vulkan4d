// TODO - Our own m4 struct
// TODO - I figure optimally, we probably want to precompute proj * view every
// frame as our uniform buffer, then push the model as a push constant.
struct vk_ubo
{
	alignas(16) mat4 model;
	alignas(16) mat4 view;
	alignas(16) mat4 proj;
};

struct vk_host_memory
{
	struct vk_ubo ubo;
};

struct vk_context
{
	VkInstance            instance;
	VkPhysicalDevice      physical_device;
	VkDevice              device;
	VkSurfaceKHR          surface;

	VkQueue               queue_graphics;
	VkQueue               queue_present;

	VkImageView           render_view;
	VkImage               render_image;
	// TODO - Can this be pulled into device_local_memory, and would that be
	// inadvisable?
	VkDeviceMemory        render_image_memory;
	VkSampleCountFlagBits render_samples;

	VkSwapchainKHR        swapchain;
	VkExtent2D            swap_extent;
	VkImageView           swap_views[MAX_SWAP_IMAGES];
	VkImage               swap_images[MAX_SWAP_IMAGES];
	uint32_t              swap_images_len;

	VkDescriptorSetLayout descriptor_layout;
	VkDescriptorPool      descriptor_pool;
	VkDescriptorSet       descriptor_set;
	VkPipeline            pipeline;
	VkPipelineLayout      pipeline_layout;

	VkBuffer              device_local_buffer;
	VkDeviceMemory        device_local_memory;

	VkBuffer              host_visible_buffer;
	VkDeviceMemory        host_visible_memory;
	void*                 host_visible_mapped;

	uint32_t              vertex_buffer_offset;
	uint32_t              index_buffer_offset;

	VkCommandPool         command_pool;
	VkCommandBuffer       command_buffer;

	VkSemaphore           semaphore_image_available;
	VkSemaphore           semaphore_render_finished;
};

struct vk_platform
{
	VkResult(*create_surface_callback)(struct vk_context* vk, void* context);
	void*   context;
	char**  window_extensions;
	uint8_t window_extensions_len;
};


struct vk_create_swapchain_result
{
	VkSurfaceFormatKHR surface_format;
};

struct vk_vertex
{
	struct v3 pos;
	struct v3 color;
};
