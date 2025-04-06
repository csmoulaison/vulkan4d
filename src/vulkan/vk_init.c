VkShaderModule vk_create_shader_module(VkDevice device, const char* fname)
{
	FILE* file = fopen(fname, "r");
	if(!file)
	{
		printf("Failed to open file: %s\n", fname);
		PANIC();
	}
	fseek(file, 0, SEEK_END);
	uint32_t fsize = ftell(file);
	fseek(file, 0, SEEK_SET);
	char src[fsize];

	char c;
	uint32_t i = 0;
	while((c = fgetc(file)) != EOF)
	{
		src[i] = c;
		i++;
	}
	fclose(file);
	
	VkShaderModuleCreateInfo info = {};
	info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.codeSize = fsize;
	info.pCode    = (uint32_t*)src;

	VkShaderModule module;
	VkResult res = vkCreateShaderModule(device, &info, 0, &module);
	if(res != VK_SUCCESS)
	{
		printf("Error %i: Failed to create shader module.\n", res);
		PANIC();
	}
	
	return module;
}

uint32_t vk_get_memory_type(struct vk_context* vk, uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(vk->physical_device, &mem_properties);
	
	for(uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
	{
		if((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	printf("Failed to find suitable memory type for buffer.\n");
	PANIC();
	return 0;
}

void vk_allocate_buffer(
	struct vk_context* vk,
	VkBuffer* buffer,
	VkDeviceMemory* memory,
	VkDeviceSize size, 
	VkBufferUsageFlags usage, 
	VkMemoryPropertyFlags properties)
{
	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.size = size;
	buf_info.usage = usage;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult res = vkCreateBuffer(vk->device, &buf_info, 0, buffer);
	if(res != VK_SUCCESS)
	{
		printf("Error %i: Failed to create buffer.\n", res);
		PANIC();
	}

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(vk->device, *buffer, &mem_reqs);

	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(vk->physical_device, &mem_properties);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_reqs.size;
	alloc_info.memoryTypeIndex = vk_get_memory_type(
		vk,
		mem_reqs.memoryTypeBits, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	res = vkAllocateMemory(vk->device, &alloc_info, 0, memory);
	if(res != VK_SUCCESS)
	{
		printf("Error %i: Failed to allocate buffer memory.\n", res);
	}
	vkBindBufferMemory(vk->device, *buffer, *memory, 0);
}

struct vk_create_swapchain_result vk_create_swapchain(struct vk_context* vk, bool recreate)
{
	struct vk_create_swapchain_result result;

	if(recreate) 
	{
		vkDeviceWaitIdle(vk->device);
		for(uint32_t i = 0; i < vk->swap_images_len; i++)
		{
			vkDestroyImageView(vk->device, vk->swap_views[i], 0);
		}
		vkDestroySwapchainKHR(vk->device, vk->swapchain, 0);

		vkDestroyImage(vk->device, vk->render_image, 0);
		vkDestroyImageView(vk->device, vk->render_view, 0);
		vkFreeMemory(vk->device, vk->render_image_memory, 0);

		vkDestroySemaphore(vk->device, vk->semaphore_image_available, 0);
		vkDestroySemaphore(vk->device, vk->semaphore_render_finished, 0);
	}

	// Query surface capabilities.
	uint32_t image_count = 0;
	VkSurfaceTransformFlagBitsKHR pre_transform;
	{
		VkSurfaceCapabilitiesKHR abilities;
		VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk->physical_device, vk->surface, &abilities);
		if(res != VK_SUCCESS) 
		{
			printf("Error %i: Failed to get physical device surface capabilities.\n", res);
			PANIC();
		}

		// XXX - See what we can do about this, or whether it's necessary to do
		// anything different.
		vk->swap_extent.width = abilities.maxImageExtent.width;
		vk->swap_extent.height = abilities.maxImageExtent.height;

		if(abilities.minImageExtent.width  > vk->swap_extent.width
		|| abilities.maxImageExtent.width  < vk->swap_extent.width
		|| abilities.minImageExtent.height > vk->swap_extent.height
		|| abilities.maxImageExtent.height < vk->swap_extent.height) 
		{
			printf("Error %i: Surface KHR extents are not compatible with configured surface sizes.\n", res);
			PANIC();
		}

		image_count = abilities.minImageCount + 1;
		if(abilities.maxImageCount > 0 && image_count > abilities.maxImageCount) 
		{
			image_count = abilities.maxImageCount;
		}

		pre_transform = abilities.currentTransform;
	}

	// Choose surface format.
	{
		uint32_t formats_len;
		vkGetPhysicalDeviceSurfaceFormatsKHR(vk->physical_device, vk->surface, &formats_len, 0);
		if(formats_len == 0) 
		{
			printf("Physical device doesn't support any formats?\n");
			PANIC();
		}

		VkSurfaceFormatKHR formats[formats_len];
		vkGetPhysicalDeviceSurfaceFormatsKHR(vk->physical_device, vk->surface, &formats_len, formats);

		result.surface_format = formats[0];
		for(int i = 0; i < formats_len; i++) 
		{
			if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
			{
				result.surface_format = formats[i];
				break;
			}
		}
	}

	// Choose presentation mode.
	// 
	// Default to VK_PRESENT_MODE_FIFO_KHR, as this is the only mode required to
	// be supported by the spec.
	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	{
		uint32_t modes_len;
		VkResult res = vkGetPhysicalDeviceSurfacePresentModesKHR(vk->physical_device, vk->surface, &modes_len, 0);
		if(res != VK_SUCCESS) 
		{
			printf("Error %i: Failed to get number of physical device surface present modes.\n", res);
			PANIC();
		}

		VkPresentModeKHR modes[modes_len];
		res = vkGetPhysicalDeviceSurfacePresentModesKHR(vk->physical_device, vk->surface, &modes_len, modes);
		if(res != VK_SUCCESS) 
		{
			printf("Error %i: Failed to get physical device surface present modes.\n", res);
			PANIC();
		}

		for(int i = 0; i < modes_len; i++) 
		{
			if(modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) 
			{
				present_mode = modes[i];
				break;
			}
		}
	}

	VkSwapchainCreateInfoKHR info = {};
	info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.pNext                 = 0;
	info.flags                 = 0; // TODO mutable format or any other flags?
	info.surface               = vk->surface;
	info.minImageCount         = image_count; // TODO get this value.
	info.imageFormat           = result.surface_format.format;
	info.imageColorSpace       = result.surface_format.colorSpace;
	info.imageExtent           = vk->swap_extent;
	info.imageArrayLayers      = 1;
	info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // TODO probably right, but we'll see.
	info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE; // TODO needs to be CONCURRENT if compute is in different family from present
	info.queueFamilyIndexCount = 0; // Not used in exclusive mode. Need to check for concurrent.
	info.pQueueFamilyIndices   = 0; // Also not used in exclusive mode, see above.
	info.preTransform          = pre_transform;
	info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode           = present_mode;
	info.clipped               = VK_TRUE;
	info.oldSwapchain          = VK_NULL_HANDLE;

#if VK_IMMEDIATE
		info.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
#endif

	VkResult res = vkCreateSwapchainKHR(vk->device, &info, 0, &vk->swapchain);
	if(res != VK_SUCCESS) 
	{
		printf("Error %i: Failed to create swapchain.\n", res);
		PANIC();
	}

	// Get swapchain images
	res = vkGetSwapchainImagesKHR(vk->device, vk->swapchain, &vk->swap_images_len, 0);
	if(res != VK_SUCCESS) 
	{
		printf("Error %i: Failed to get number of swapchain images.\n", res);
		PANIC();
	}

	res = vkGetSwapchainImagesKHR(vk->device, vk->swapchain, &vk->swap_images_len, vk->swap_images);
	if(res != VK_SUCCESS) 
	{
		printf("Error %i: Failed to get swapchain images.\n", res);
		PANIC();
	}

	// Create image views.
	for(int i = 0; i < vk->swap_images_len; i++) 
	{
		VkImageViewCreateInfo info = {};
		info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image                           = vk->swap_images[i];
		info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
		info.format                          = result.surface_format.format;
		info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.baseMipLevel   = 0;
		info.subresourceRange.levelCount     = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount     = 1;
		
		res = vkCreateImageView(vk->device, &info, 0, &vk->swap_views[i]);
		if(res != VK_SUCCESS) 
		{
			printf("Error %i: Failed to create image views.\n", res);
			PANIC();
		}
	}

	// Create render image view for multisampling
	VkImageCreateInfo render_info = {};
	render_info.sType 		  = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	render_info.imageType     = VK_IMAGE_TYPE_2D;
	render_info.format        = result.surface_format.format;
	render_info.extent        = (VkExtent3D){vk->swap_extent.width, vk->swap_extent.height, 1};
	render_info.mipLevels     = 1;
	render_info.arrayLayers   = 1;
	render_info.samples       = vk->render_samples;
	render_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
	render_info.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	render_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	res = vkCreateImage(vk->device, &render_info, 0, &vk->render_image);
	if(res != VK_SUCCESS) 
	{
		printf("Error %i: Failed to create render image.\n", res);
		PANIC();
	}

	VkMemoryRequirements mem_reqs = {};
	vkGetImageMemoryRequirements(vk->device, vk->render_image, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_reqs.size;
	alloc_info.memoryTypeIndex = vk_get_memory_type(
		vk,
		mem_reqs.memoryTypeBits, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	res = vkAllocateMemory(vk->device, &alloc_info, 0, &vk->render_image_memory);
	if(res != VK_SUCCESS) 
	{
		printf("Error %i: Failed to allocate render image memory..\n", res);
		PANIC();
	}
	res = vkBindImageMemory(vk->device, vk->render_image, vk->render_image_memory, 0);
	if(res != VK_SUCCESS) 
	{
		printf("Error %i: Failed to bind render image memory.\n", res);
		PANIC();
	}

	VkImageViewCreateInfo render_view_info = {};
	render_view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	render_view_info.image                           = vk->render_image;
	render_view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	render_view_info.format                          = result.surface_format.format;
	render_view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	render_view_info.subresourceRange.baseMipLevel   = 0;
	render_view_info.subresourceRange.levelCount     = 1;
	render_view_info.subresourceRange.baseArrayLayer = 0;
	render_view_info.subresourceRange.layerCount     = 1;

	res = vkCreateImageView(vk->device, &render_view_info, 0, &vk->render_view);
	if(res != VK_SUCCESS) 
	{
		printf("Error %i: Failed to create render image view.\n", res);
		PANIC();
	}

	// Create synchronization primitives
	{
		VkSemaphoreCreateInfo semaphore_info = {};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkResult res = vkCreateSemaphore(vk->device, &semaphore_info, 0, &vk->semaphore_image_available);
		if(res != VK_SUCCESS)
		{
			printf("Error %i: Failed to create image available semaphore.\n", res);
			PANIC();
		}
		res = vkCreateSemaphore(vk->device, &semaphore_info, 0, &vk->semaphore_render_finished);
		if(res != VK_SUCCESS)
		{
			printf("Error %i: Failed to create render finished semaphore.\n", res);
			PANIC();
		}
	}

	return result;
}

struct vk_context vk_init(struct vk_platform* platform)
{
	struct vk_context vk;

	// Create instance.
	{
		VkApplicationInfo app = {};
		app.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app.pNext              = 0;
		app.pApplicationName   = PROGRAM_NAME;
		app.applicationVersion = 1;
		app.pEngineName        = 0;
		app.engineVersion      = 0;
		app.apiVersion         = VK_API_VERSION_1_3;

		uint32_t exts_len = platform->window_extensions_len;

#if VK_DEBUG
			exts_len++;
			char* debug_ext = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#endif

		const char* exts[exts_len];
		for(int i = 0; i < platform->window_extensions_len; i++) {
			exts[i] = platform->window_extensions[i];
		}

#if VK_DEBUG
			exts[exts_len - 1] = debug_ext;
#endif

		VkInstanceCreateInfo info = {};
		info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		info.pNext            = 0;
		info.flags            = 0;
		info.pApplicationInfo = &app;

		uint32_t layers_len = 0;

		#ifdef VK_DEBUG
			layers_len++;
		#endif

		const char* layers[layers_len];

#if VK_DEBUG
			layers[layers_len - 1] = "VK_LAYER_KHRONOS_validation";
#endif

		info.enabledLayerCount = layers_len;
		info.ppEnabledLayerNames = layers;

		info.enabledExtensionCount = exts_len;
		info.ppEnabledExtensionNames = exts;

		VkResult res = vkCreateInstance(&info, 0, &vk.instance);
		if(res != VK_SUCCESS) 
		{
			printf("Error %i: Failed to create instance.\n", res);
			PANIC();
		}
	}

	// Create surface.
	{
		bool res = platform->create_surface_callback(&vk, platform->context);
		if(res != VK_SUCCESS) 
		{
			printf("Error %i: Failed to create platform surface.\n", res);
			PANIC();
		}
	}

	// Create physical device.
	uint32_t graphics_family_idx = 0;
	{
		uint32_t devices_len;
		if(vkEnumeratePhysicalDevices(vk.instance, &devices_len, 0) != VK_SUCCESS) 
		{
			printf("Failed to enumerate physical devices to get device count.\n");
			PANIC();
		}
		VkPhysicalDevice devices[devices_len];
		if(vkEnumeratePhysicalDevices(vk.instance, &devices_len, devices) != VK_SUCCESS) 
		{
			printf("Failed to enumerate physical devices to get devices.\n");
			PANIC();
		}

		vk.physical_device = 0;
		for(int i = 0; i < devices_len; i++) 
		{
			// Check queue families
			uint32_t fams_len;
			vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &fams_len, 0);
			VkQueueFamilyProperties fams[fams_len];
			vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &fams_len, fams);

			//bool compute = false;
			bool graphics = false;
			for(int j = 0; j < fams_len; j++) 
			{
				/*
				if(fams[j].queueFlags & VK_QUEUE_COMPUTE_BIT) 
				{
					compute = true;
					compute_family_idx = j;
				}
				*/
				if(fams[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) 
				{
					graphics = true;
					graphics_family_idx = j;
				}
			}
			if(!graphics) 
			{
				continue;
			}

			// Check device extensions.
			uint32_t exts_len;
			vkEnumerateDeviceExtensionProperties(devices[i], 0, &exts_len, 0);
			VkExtensionProperties exts[exts_len];
			vkEnumerateDeviceExtensionProperties(devices[i], 0, &exts_len, exts);

			bool swapchain = false;
			bool dynamic = false;
			for(int j = 0; j < exts_len; j++) 
			{
				if(strcmp(exts[j].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) 
				{
					swapchain = true;
					continue;
				}
				if(strcmp(exts[j].extensionName, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME) == 0) 
				{
					dynamic = true;
					continue;
				}
			}
			if(!swapchain || !dynamic) 
			{
				continue;
			}

			vk.physical_device = devices[i];

			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(vk.physical_device, &properties);
			VkSampleCountFlags sample_counts = properties.limits.framebufferColorSampleCounts; //& properties.limits.framebufferDepthSampleCounts;
			if(sample_counts & VK_SAMPLE_COUNT_64_BIT) 
			{ 
				vk.render_samples = VK_SAMPLE_COUNT_64_BIT; 
			} 
			else if(sample_counts & VK_SAMPLE_COUNT_32_BIT)
			{
				vk.render_samples = VK_SAMPLE_COUNT_32_BIT; 
			}
			else if(sample_counts & VK_SAMPLE_COUNT_16_BIT)
			{
				vk.render_samples = VK_SAMPLE_COUNT_16_BIT; 
			}
			else if(sample_counts & VK_SAMPLE_COUNT_8_BIT)
			{
				vk.render_samples = VK_SAMPLE_COUNT_8_BIT; 
			}
			else if(sample_counts & VK_SAMPLE_COUNT_4_BIT)
			{
				vk.render_samples = VK_SAMPLE_COUNT_4_BIT; 
			}
			else if(sample_counts & VK_SAMPLE_COUNT_2_BIT)
			{
				vk.render_samples = VK_SAMPLE_COUNT_2_BIT; 
			}
			else
			{
				vk.render_samples = VK_SAMPLE_COUNT_1_BIT;
			}
		}

		// Exit if we haven't found an eligible device.
		if(!vk.physical_device) 
		{
			printf("No suitable physical device.\n");
			PANIC();
		}
	}

	// Create logical device.
	{ 
		VkDeviceQueueCreateInfo queue = {};
		queue.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue.pNext            = 0;
		queue.flags            = 0;
		queue.queueFamilyIndex = graphics_family_idx;
		queue.queueCount       = 1; // 1 because only 1 queue, right?
		float priority         = 1.0f;
	 	queue.pQueuePriorities = &priority;

	 	VkPhysicalDeviceDynamicRenderingFeatures dynamic_features = {};
		dynamic_features.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		dynamic_features.pNext            = 0;
		dynamic_features.dynamicRendering = VK_TRUE;


		VkPhysicalDeviceFeatures2 features = {};
		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features.pNext = &dynamic_features;
		vkGetPhysicalDeviceFeatures2(vk.physical_device, &features);
		
		VkDeviceCreateInfo info = {};
		info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	 	info.pNext                   = &features;
	 	info.flags                   = 0;
	 	info.queueCreateInfoCount    = 1;
	 	info.pQueueCreateInfos       = &queue;
	 	const char* device_exts[2] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME };
	 	info.enabledExtensionCount   = 2;
	 	info.ppEnabledExtensionNames = device_exts;

	 	VkResult res = vkCreateDevice(vk.physical_device, &info, 0, &vk.device);
		if(res != VK_SUCCESS) 
		{
			printf("Error %i: Failed to create logical device.\n", res);
			PANIC();
		}

		vkGetDeviceQueue(vk.device, graphics_family_idx, 0, &vk.queue_graphics);
	}

	// Create swapchain, images, and image views. This has been abstracted to allow
	// swapchain recreation after initialization in the case of window resize, for
	// example.
	// XXX - If I was being really pedantic, we would also include pipeline
	// recreation as the surface format could theoretically also change at runtime.
	VkSurfaceFormatKHR surface_format;
	{
		struct vk_create_swapchain_result res = vk_create_swapchain(&vk, false);
		surface_format = res.surface_format;
	}

	// Allocate host visible memory buffer.
	{
		VkDeviceSize buf_size = sizeof(struct vk_host_memory);

		vk_allocate_buffer(
			&vk,
			&vk.host_visible_buffer,
			&vk.host_visible_memory,
			buf_size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		vkMapMemory(vk.device, vk.host_visible_memory, 0, buf_size, 0, (void*)&vk.host_visible_mapped);
	}

	// Create descriptor set layout, pool, and sets.
	{
		VkDescriptorSetLayoutBinding ubo_binding = {};
		ubo_binding.binding         = 0;
		ubo_binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_binding.descriptorCount = 1;
		ubo_binding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo layout_info = {};
		layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = 1;
		layout_info.pBindings    = &ubo_binding;

		VkResult res = vkCreateDescriptorSetLayout(vk.device, &layout_info, 0, &vk.descriptor_layout);
		if(res != VK_SUCCESS)
		{
			printf("Error %i: Failed to create descriptor set layout.\n", res);
			PANIC();
		}

		VkDescriptorPoolSize pool_size = {};
		pool_size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_size.descriptorCount = 1;

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.poolSizeCount = 1;
		pool_info.pPoolSizes    = &pool_size;
		pool_info.maxSets       = 1;

		res = vkCreateDescriptorPool(vk.device, &pool_info, 0, &vk.descriptor_pool);
		if(res != VK_SUCCESS)
		{
			printf("Error %i: Failed to create descriptor pool.\n", res);
			PANIC();
		}

		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool     = vk.descriptor_pool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts        = &vk.descriptor_layout;

		res = vkAllocateDescriptorSets(vk.device, &alloc_info, &vk.descriptor_set);
		if(res != VK_SUCCESS)
		{
			printf("Error %i: Failed to create descriptor set.\n", res);
			PANIC();
		}

		VkDescriptorBufferInfo buf_info = {};
		buf_info.buffer = vk.host_visible_buffer;
		buf_info.offset = 0;
		buf_info.range  = sizeof(struct vk_host_memory);

		VkWriteDescriptorSet desc_write = {};
		desc_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc_write.dstSet          = vk.descriptor_set;
		desc_write.dstBinding      = 0;
		desc_write.dstArrayElement = 0;
		desc_write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		desc_write.descriptorCount = 1;
		desc_write.pBufferInfo     = &buf_info;

		vkUpdateDescriptorSets(vk.device, 1, &desc_write, 0, 0);
	}
	
	// Create graphics pipeline.
	{
		VkShaderModule shader_vert = vk_create_shader_module(vk.device, "shaders/vert.spv");
		VkShaderModule shader_frag = vk_create_shader_module(vk.device, "shaders/frag.spv");

		size_t shader_infos_len = 2;
		VkPipelineShaderStageCreateInfo shader_infos[shader_infos_len] = {};

		shader_infos[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_infos[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
		shader_infos[0].module = shader_vert;
		shader_infos[0].pName  = "main";

		shader_infos[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_infos[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_infos[1].module = shader_frag;
		shader_infos[1].pName  = "main";

		size_t bind_descriptions_len = 1;
		VkVertexInputBindingDescription bind_descriptions[bind_descriptions_len] = {};
		bind_descriptions[0].binding   = 0;
		bind_descriptions[0].stride    = sizeof(struct vk_vertex);
		bind_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		size_t attr_descriptions_len = 2;
		VkVertexInputAttributeDescription attr_descriptions[attr_descriptions_len] = {};

		attr_descriptions[0].binding  = 0;
		attr_descriptions[0].location = 0;
		attr_descriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attr_descriptions[0].offset   = offsetof(struct vk_vertex, pos);

		attr_descriptions[1].binding  = 0;
		attr_descriptions[1].location = 1;
		attr_descriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attr_descriptions[1].offset   = offsetof(struct vk_vertex, color);

		VkPipelineVertexInputStateCreateInfo vert_input_info = {};
		vert_input_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vert_input_info.vertexBindingDescriptionCount   = bind_descriptions_len;
		vert_input_info.pVertexBindingDescriptions      = bind_descriptions;
		vert_input_info.vertexAttributeDescriptionCount = attr_descriptions_len;
		vert_input_info.pVertexAttributeDescriptions    = attr_descriptions;

		VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
		input_assembly_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_info.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly_info.primitiveRestartEnable = VK_FALSE;

		// VOLATILE - Length must match dynamic_info.dynamicStateCount.
		VkDynamicState dyn_states[2] = 
		{
			VK_DYNAMIC_STATE_VIEWPORT, 
			VK_DYNAMIC_STATE_SCISSOR 
		};
		VkPipelineDynamicStateCreateInfo dynamic_info = {};
		dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_info.dynamicStateCount = 2;
		dynamic_info.pDynamicStates = dyn_states;

		VkViewport viewport = {};
		viewport.x        = 0.0f;
		viewport.y        = 0.0f;
		viewport.width    = (float)vk.swap_extent.width;
		viewport.width    = (float)vk.swap_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = (VkOffset2D){0, 0};
		scissor.extent = vk.swap_extent;

		VkPipelineViewportStateCreateInfo viewport_info = {};
		viewport_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_info.viewportCount = 1;
		viewport_info.pViewports    = &viewport;
		viewport_info.scissorCount  = 1;
		viewport_info.pScissors     = &scissor;

		VkPipelineRasterizationStateCreateInfo raster_info = {};
		raster_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		raster_info.depthClampEnable        = VK_FALSE;
		raster_info.rasterizerDiscardEnable = VK_FALSE;
		raster_info.polygonMode             = VK_POLYGON_MODE_FILL;
		raster_info.lineWidth               = 1.0f;
		raster_info.cullMode                = VK_CULL_MODE_BACK_BIT;
		raster_info.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		raster_info.depthBiasEnable         = VK_FALSE;
		raster_info.depthBiasConstantFactor = 0.0f;
		raster_info.depthBiasClamp          = 0.0f;
		raster_info.depthBiasSlopeFactor    = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisample_info = {};
		multisample_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_info.sampleShadingEnable   = VK_FALSE;
		multisample_info.rasterizationSamples  = vk.render_samples;

		VkPipelineColorBlendAttachmentState color_blend_attachment = {};
		color_blend_attachment.colorWriteMask = 
			VK_COLOR_COMPONENT_R_BIT | 
			VK_COLOR_COMPONENT_G_BIT | 
			VK_COLOR_COMPONENT_B_BIT | 
			VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable    = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo color_blend_info = {};
		color_blend_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_info.logicOpEnable     = VK_FALSE;
		color_blend_info.attachmentCount   = 1;
		color_blend_info.pAttachments      = &color_blend_attachment;
		color_blend_info.blendConstants[0] = 0.0f;
		color_blend_info.blendConstants[1] = 0.0f;
		color_blend_info.blendConstants[2] = 0.0f;
		color_blend_info.blendConstants[3] = 0.0f;

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &vk.descriptor_layout;

		VkResult res = vkCreatePipelineLayout(vk.device, &pipeline_layout_info, 0, &vk.pipeline_layout);
		if(res != VK_SUCCESS)
		{
			printf("Error %i: Failed to created pipeline layout.\n", res);
			PANIC();
		}

		VkPipelineRenderingCreateInfoKHR pipeline_render_info = {};
		pipeline_render_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR; 
		pipeline_render_info.pNext                   = VK_NULL_HANDLE; 
		pipeline_render_info.colorAttachmentCount    = 1; 
		pipeline_render_info.pColorAttachmentFormats = &surface_format.format; 
		pipeline_render_info.depthAttachmentFormat   = 0; // TODO - is this okay if not depth buffering?
		pipeline_render_info.stencilAttachmentFormat = 0;

		VkGraphicsPipelineCreateInfo pipeline_info = {};
		pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.pNext               = &pipeline_render_info;
		pipeline_info.renderPass          = VK_NULL_HANDLE;
		pipeline_info.pInputAssemblyState = &input_assembly_info;
		pipeline_info.pRasterizationState = &raster_info;
		pipeline_info.pColorBlendState    = &color_blend_info;
		pipeline_info.pMultisampleState   = &multisample_info;
		pipeline_info.pViewportState      = &viewport_info;
		pipeline_info.pDepthStencilState  = 0;
		pipeline_info.pDynamicState       = &dynamic_info;
		pipeline_info.pVertexInputState   = &vert_input_info;
		pipeline_info.stageCount          = shader_infos_len;
		pipeline_info.pStages             = shader_infos;
		pipeline_info.layout              = vk.pipeline_layout;

		res = vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipeline_info, 0, &vk.pipeline);
		if(res != VK_SUCCESS)
		{
			printf("Error %i: Failed to create graphics pipeline.\n", res);
		}

		vkDestroyShaderModule(vk.device, shader_vert, 0);
		vkDestroyShaderModule(vk.device, shader_frag, 0);
	}

	// Create command pool and allocate command buffers
	{
		VkCommandPoolCreateInfo pool_info = {};
		pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = graphics_family_idx;

		VkResult res = vkCreateCommandPool(vk.device, &pool_info, 0, &vk.command_pool);
		if(res != VK_SUCCESS)
		{
			printf("Error %i: Failed to create command pool.\n", res);
			PANIC();
		}

		VkCommandBufferAllocateInfo buf_info = {};
		buf_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		buf_info.commandPool        = vk.command_pool;
		buf_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		buf_info.commandBufferCount = 1;

		res = vkAllocateCommandBuffers(vk.device, &buf_info, &vk.command_buffer);
		if(res != VK_SUCCESS)
		{
			printf("Error %i: Failed to allocate command buffer.\n", res);
			PANIC();
		}
	}

	// Allocate device local memory buffer
	{
		size_t vertex_buffer_size = sizeof(struct vk_vertex) * VERTICES_LEN;
		size_t index_buffer_size = sizeof(uint16_t) * INDICES_LEN;
		VkDeviceSize buf_size = vertex_buffer_size + index_buffer_size;

		VkBuffer staging_buf;
		VkDeviceMemory staging_buf_mem;
		vk_allocate_buffer(
			&vk,
			&staging_buf,
			&staging_buf_mem,
			buf_size, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		vk.vertex_buffer_offset = 0;
		vk.index_buffer_offset = vertex_buffer_size;

		void* buf_data;
		vkMapMemory(vk.device, staging_buf_mem, 0, buf_size, 0, &buf_data);
		{
			memcpy(buf_data + vk.vertex_buffer_offset, vertices, vertex_buffer_size);
			memcpy(buf_data + vk.index_buffer_offset, indices, index_buffer_size);
		}
		vkUnmapMemory(vk.device, staging_buf_mem);

		vk_allocate_buffer(
			&vk,
			&vk.device_local_buffer,
			&vk.device_local_memory,
			buf_size, 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkCommandBufferAllocateInfo alloc_info = {};
		alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandPool        = vk.command_pool;
		alloc_info.commandBufferCount = 1;

		VkCommandBuffer cmd_buf;
		vkAllocateCommandBuffers(vk.device, &alloc_info, &cmd_buf);

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(cmd_buf, &begin_info);
		{
			VkBufferCopy copy = {};
			copy.size = buf_size;
			vkCmdCopyBuffer(cmd_buf, staging_buf, vk.device_local_buffer, 1, &copy);
		}
		vkEndCommandBuffer(cmd_buf);

		VkSubmitInfo submit_info = {};
		submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers    = &cmd_buf;

		// TODO - We are using the graphics queue for this presently, but might we
		// want to use a transfer queue for this?
		// I'm not sure if there's any possible performance boost, and I'd rather not
		// do it just for the sake of it.
		// 
		// High level on how this might be done at the following link:
		// https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer
		vkQueueSubmit(vk.queue_graphics, 1, &submit_info, VK_NULL_HANDLE);
		vkQueueWaitIdle(vk.queue_graphics);

		vkFreeCommandBuffers(vk.device, vk.command_pool, 1, &cmd_buf);
		vkDestroyBuffer(vk.device, staging_buf, 0);
		vkFreeMemory(vk.device, staging_buf_mem, 0);
	}

	return vk;
}
