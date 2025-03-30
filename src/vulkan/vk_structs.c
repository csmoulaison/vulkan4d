struct vk_per_frame
{
	VkCommandBuffer cmd_buffer;
	VkSemaphore semaphore_image_available;
	VkSemaphore semaphore_render_finished;
};

struct vk_context
{
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical_device;
	VkDevice device;

	//VkQueue queue_compute;
	VkQueue queue_graphics;
	VkQueue queue_present;

	VkSwapchainKHR swapchain;
	VkImage swap_images[MAX_SWAP_IMAGES];
	VkImageView swap_views[MAX_SWAP_IMAGES];
	VkExtent2D swap_extent;
	uint32_t images_len;

	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;

	VkCommandPool cmd_pool;

	uint32_t frame_cur;
	struct vk_per_frame frames[MAX_IN_FLIGHT_FRAMES];
};
