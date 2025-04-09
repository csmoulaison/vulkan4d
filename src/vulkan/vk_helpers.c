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

uint32_t vk_get_memory_type(
	VkPhysicalDevice      physical_device, 
	uint32_t              type_filter, 
	VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
	
	for(uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
	{
		if((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	printf("Failed to find suitable memory type for buffer.\n");
	PANIC();
}

void vk_allocate_buffer(
	VkDevice              device,
	VkPhysicalDevice      physical_device,
	VkBuffer*             buffer,
	VkDeviceMemory*       memory,
	VkDeviceSize          size, 
	VkBufferUsageFlags    usage, 
	VkMemoryPropertyFlags properties)
{
	VkBufferCreateInfo buf_info = {};
	buf_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.size        = size;
	buf_info.usage       = usage;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult res = vkCreateBuffer(device, &buf_info, 0, buffer);
	if(res != VK_SUCCESS)
	{
		printf("Error %i: Failed to create buffer.\n", res);
		PANIC();
	}

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(device, *buffer, &mem_reqs);

	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize  = mem_reqs.size;
	alloc_info.memoryTypeIndex = vk_get_memory_type(
		physical_device,
		mem_reqs.memoryTypeBits, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	res = vkAllocateMemory(device, &alloc_info, 0, memory);
	if(res != VK_SUCCESS)
	{
		printf("Error %i: Failed to allocate buffer memory.\n", res);
	}
	vkBindBufferMemory(device, *buffer, *memory, 0);
}

void vk_allocate_image(
	VkDevice          device,
	VkPhysicalDevice  physical_device,
	VkImage*          image,
	VkDeviceMemory*   memory,
	uint32_t          width,
	uint32_t          height,
	VkFormat          format,
	uint32_t          samples,
	VkImageUsageFlags usage_mask)
{
	VkImageCreateInfo image_info = {};
	image_info.sType 		 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType     = VK_IMAGE_TYPE_2D;
	image_info.format        = format;
	image_info.extent.width  = width;
	image_info.extent.height = height;
	image_info.extent.depth  = 1;
	image_info.mipLevels     = 1;
	image_info.arrayLayers   = 1;
	image_info.samples       = samples;
	image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
	image_info.usage         = usage_mask;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkResult res = vkCreateImage(device, &image_info, 0, image);
	if(res != VK_SUCCESS) 
	{
		printf("Error %i: Failed to create image.\n", res);
		PANIC();
	}

	VkMemoryRequirements mem_reqs = {};
	vkGetImageMemoryRequirements(device, *image, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_reqs.size;
	alloc_info.memoryTypeIndex = vk_get_memory_type(
		physical_device,
		mem_reqs.memoryTypeBits, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	res = vkAllocateMemory(device, &alloc_info, 0, memory);
	if(res != VK_SUCCESS) 
	{
		printf("Error %i: Failed to allocate image memory..\n", res);
		PANIC();
	}
	res = vkBindImageMemory(device, *image, *memory, 0);
	if(res != VK_SUCCESS) 
	{
		printf("Error %i: Failed to bind image memory.\n", res);
		PANIC();
	}
}

void vk_create_image_view(
	VkDevice           device,
	VkImageView*       view,
	VkImage            image,
	VkFormat           format,
	VkImageAspectFlags aspect_mask)
{
	VkImageViewCreateInfo view_info = {};
	view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image                           = image;
	view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format                          = format;
	view_info.subresourceRange.aspectMask     = aspect_mask;
	view_info.subresourceRange.baseMipLevel   = 0;
	view_info.subresourceRange.levelCount     = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount     = 1;

	VkResult res = vkCreateImageView(device, &view_info, 0, view);
	if(res != VK_SUCCESS) 
	{
		printf("Error %i: Failed to create image view.\n", res);
		PANIC();
	}
}

void vk_allocate_image_and_view(
	VkDevice           device,
	VkPhysicalDevice   physical_device,
	VkImage*           image,
	VkDeviceMemory*    memory,
	VkImageView*       view,
	uint32_t           width,
	uint32_t           height,
	VkFormat           format,
	uint32_t           samples,
	VkImageUsageFlags  usage_mask,
	VkImageAspectFlags aspect_mask)
{
	vk_allocate_image(device, physical_device, image, memory, width, height, format, samples, usage_mask);
	vk_create_image_view(device, view, *image, format, aspect_mask);
}

void vk_allocate_texture(
	VkDevice         device,
	VkPhysicalDevice physical_device,
	VkImage*         image,
	VkDeviceMemory*  memory,
	char*            fname)
{
	int32_t tex_w;
	int32_t tex_h;
	int32_t tex_channels;
	stbi_uc* pixels = stbi_load(fname, &tex_w, &tex_h, &tex_channels, STBI_rgb_alpha);
	if(!pixels)
	{
		printf("Failed to load image file: %s.\n", fname);
		PANIC();
	}

	VkDeviceSize img_size = tex_w * tex_h * 4;
	VkBuffer staging_buf;
	VkDeviceMemory staging_mem;

	vk_allocate_buffer(
		device, 
		physical_device,
		&staging_buf, 
		&staging_mem,
		img_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* data;
	vkMapMemory(device, staging_mem, 0, img_size, 0, &data);
	memcpy(data, pixels, (size_t)img_size);
	vkUnmapMemory(device, staging_mem);

	stbi_image_free(pixels);

	vk_allocate_image(
		device,
		physical_device,
		image, 
		memory,
		tex_w,
		tex_h,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
}
