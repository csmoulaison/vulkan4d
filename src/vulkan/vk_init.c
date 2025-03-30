VkShaderModule create_shader_module(VkDevice device, const char* fname)
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

struct vk_context vk_init(
	char** window_exts,
	uint8_t window_exts_len,
	VkResult(*create_surface_callback)(void* platform_data, struct vk_context* vk), 
	void* platform_data)
{
	struct vk_context vk;

	// Create instance
	{
		VkApplicationInfo app = {};
		app.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app.pNext              = 0;
		app.pApplicationName   = PROGRAM_NAME;
		app.applicationVersion = 1;
		app.pEngineName        = 0;
		app.engineVersion      = 0;
		app.apiVersion         = VK_API_VERSION_1_3;

		uint32_t exts_len = window_exts_len;

		// TODO - Only for debug.
		exts_len++;
		char* debug_ext = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

		const char* exts[exts_len];
		for(int i = 0; i < window_exts_len; i++) {
			exts[i] = window_exts[i];
		}

		// TODO - Only for debug.
		exts[exts_len - 1] = debug_ext;

		VkInstanceCreateInfo info = {};
		info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		info.pNext            = 0;
		info.flags            = 0;
		info.pApplicationInfo = &app;

		const char* validation_layers[1] = { "VK_LAYER_KHRONOS_validation" };
		info.enabledLayerCount = 1;
		info.ppEnabledLayerNames = validation_layers;

		info.enabledExtensionCount = exts_len;
		info.ppEnabledExtensionNames = exts;

		VkResult res = vkCreateInstance(&info, 0, &vk.instance);
		if(res != VK_SUCCESS) 
		{
			printf("Error %i: Failed to create instance.\n", res);
			PANIC();
		}
	}

	// Create surface
	{
		bool res = create_surface_callback(platform_data, &vk);
		if(res != VK_SUCCESS) 
		{
			printf("Error %i: Failed to create platform surface.\n", res);
			PANIC();
		}
	}

	// Create physical device
	//uint32_t compute_family_idx = 0;
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
			// Not needed: Get device properties
			// VkPhysicalDeviceProperties props;
			// vkGetPhysicalDeviceProperties(devices[i], &props);

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

			// Check device extensions
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
		}

		// Exit if we haven't found an eligible device
		if(!vk.physical_device) 
		{
			printf("No suitable physical device.\n");
			PANIC();
		}
	}

	// Create logical device
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

	// Create swapchain, images, and image views
	VkSurfaceFormatKHR surface_format;
	{
		// Query surface capabilities
		uint32_t image_count = 0;
		VkSurfaceTransformFlagBitsKHR pre_transform;
		{
			VkSurfaceCapabilitiesKHR abilities;
			VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physical_device, vk.surface, &abilities);
			if(res != VK_SUCCESS) 
			{
				printf("Error %i: Failed to get physical device surface capabilities.\n", res);
				PANIC();
			}

			/*
			printf("Vk Surface Info:\n");
			printf("  w: %i:\n", vk.swap_extent.width);
			printf("  h: %i:\n", vk.swap_extent.width);
			printf("  minw, maxw: %i, %i\n", abilities.minImageExtent.width, abilities.maxImageExtent.width);
			printf("  minh, maxh: %i, %i\n", abilities.minImageExtent.height, abilities.maxImageExtent.height);
			*/

			// XXX - See what we can do about this, or whether it's necessary to do
			// anything different.
			// TODO - Yep, it's definitely necessary.
			vk.swap_extent.width = abilities.maxImageExtent.width;
			vk.swap_extent.height = abilities.maxImageExtent.height;

			if(abilities.minImageExtent.width  > vk.swap_extent.width
			|| abilities.maxImageExtent.width  < vk.swap_extent.width
			|| abilities.minImageExtent.height > vk.swap_extent.height
			|| abilities.maxImageExtent.height < vk.swap_extent.height) 
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

		// Choose surface format
		{
			uint32_t formats_len;
			vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, vk.surface, &formats_len, 0);
			if(formats_len == 0) 
			{
				printf("Physical device doesn't support any formats?\n");
				PANIC();
			}

			VkSurfaceFormatKHR formats[formats_len];
			vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, vk.surface, &formats_len, formats);

			surface_format = formats[0];
			for(int i = 0; i < formats_len; i++) 
			{
				if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
				{
					surface_format = formats[i];
					break;
				}
			}
		}

		// Choose presentation mode
		// 
		// Default to VK_PRESENT_MODE_FIFO_KHR, as this is the only mode required to
		// be supported by the spec.
		VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
		{
			uint32_t modes_len;
			VkResult res = vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physical_device, vk.surface, &modes_len, 0);
			if(res != VK_SUCCESS) 
			{
				printf("Error %i: Failed to get number of physical device surface present modes.\n", res);
				PANIC();
			}

			VkPresentModeKHR modes[modes_len];
			res = vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physical_device, vk.surface, &modes_len, modes);
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
		info.surface               = vk.surface;
		info.minImageCount         = image_count; // TODO get this value.
		info.imageFormat           = surface_format.format;
		info.imageColorSpace       = surface_format.colorSpace;
		info.imageExtent           = vk.swap_extent;
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

		VkResult res = vkCreateSwapchainKHR(vk.device, &info, 0, &vk.swapchain);
		if(res != VK_SUCCESS) 
		{
			printf("Error %i: Failed to create swapchain.\n", res);
			PANIC();
		}


		// Create swapchain images and image views
		{
			// Images
			VkResult res = vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &vk.images_len, 0);
			if(res != VK_SUCCESS) 
			{
				printf("Error %i: Failed to get number of swapchain images.\n", res);
				PANIC();
			}

			res = vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &vk.images_len, vk.swap_images);
			if(res != VK_SUCCESS) 
			{
				printf("Error %i: Failed to get swapchain images.\n", res);
				PANIC();
			}

			// Image views
			for(int i = 0; i < vk.images_len; i++) 
			{
				VkImageViewCreateInfo info = {};
				info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				info.pNext                           = 0;
				info.flags                           = 0;
				info.image                           = vk.swap_images[i];
				info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
				info.format                          = surface_format.format;
				info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
				info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
				info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
				info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
				info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
				info.subresourceRange.baseMipLevel   = 0;
				info.subresourceRange.levelCount     = 1;
				info.subresourceRange.baseArrayLayer = 0;
				info.subresourceRange.layerCount     = 1;
				
				res = vkCreateImageView(vk.device, &info, 0, &vk.swap_views[i]);
				if(res != VK_SUCCESS) 
				{
					printf("Error %i: Failed to create image views.\n", res);
					PANIC();
				}
			}
		}
	}

	// Create graphics pipeline.
	//VkGraphicsPipeline pipeline;
	{
		VkShaderModule shader_vert = create_shader_module(vk.device, "shaders/vert.spv");
		VkShaderModule shader_frag = create_shader_module(vk.device, "shaders/frag.spv");
		VkPipelineShaderStageCreateInfo vert_info = {};
		vert_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
		vert_info.module = shader_vert;
		vert_info.pName  = "main";
		VkPipelineShaderStageCreateInfo frag_info = {};
		frag_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_info.module = shader_frag;
		frag_info.pName  = "main";
		// TODO - Length must equal pipeline_info.stageCount
		VkPipelineShaderStageCreateInfo shader_infos[2] = {vert_info, frag_info};

		// TODO - Load vertex data, rather than hardcode into shader as it is now.
		VkPipelineVertexInputStateCreateInfo vert_input_info = {};
		vert_input_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vert_input_info.vertexBindingDescriptionCount   = 0;
		vert_input_info.pVertexBindingDescriptions      = 0;
		vert_input_info.vertexAttributeDescriptionCount = 0;
		vert_input_info.pVertexAttributeDescriptions    = 0;

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
		raster_info.frontFace               = VK_FRONT_FACE_CLOCKWISE;
		raster_info.depthBiasEnable         = VK_FALSE;
		raster_info.depthBiasConstantFactor = 0.0f;
		raster_info.depthBiasClamp          = 0.0f;
		raster_info.depthBiasSlopeFactor    = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisample_info = {};
		multisample_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_info.sampleShadingEnable   = VK_FALSE;
		multisample_info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;

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
		pipeline_info.stageCount          = 2;
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

	{
		VkCommandPoolCreateInfo pool_info;
		pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = graphics_family_idx;

		VkResult res = vkCreateCommandPool(vk.device, &pool_info, 0, &vk.cmd_pool);
		if(res != VK_SUCCESS)
		{
			printf("Error %i: Failed to create command pool.\n", res);
			PANIC();
		}
	}

	{
		vk.frame_cur = 0;
		
		VkCommandBufferAllocateInfo buf_info = {};
		buf_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		buf_info.commandPool        = vk.cmd_pool;
		buf_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		buf_info.commandBufferCount = 1;

		VkSemaphoreCreateInfo semaphore_info = {};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for(uint32_t i = 0; i < MAX_IN_FLIGHT_FRAMES; i++)
		{
			VkResult res = vkAllocateCommandBuffers(vk.device, &buf_info, &vk.frames[i].cmd_buffer);
			if(res != VK_SUCCESS)
			{
				printf("Error %i: Failed to allocate command buffer.\n", res);
				PANIC();
			}

			res = vkCreateSemaphore(vk.device, &semaphore_info, 0, &vk.frames[i].semaphore_image_available);
			if(res != VK_SUCCESS)
			{
				printf("Error %i: Failed to create semaphore.\n", res);
				PANIC();
			}
			res = vkCreateSemaphore(vk.device, &semaphore_info, 0, &vk.frames[i].semaphore_render_finished);
			if(res != VK_SUCCESS)
			{
				printf("Error %i: Failed to create semaphore.\n", res);
				PANIC();
			}
		}
	}

	return vk;
}
