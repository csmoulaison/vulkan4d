void insert_image_memory_barrier(
	VkCommandBuffer command_buffer, 
	VkImage image, 
	VkImageLayout layout_old, 
	VkImageLayout layout_new, 
	VkPipelineStageFlagBits stage_src, 
	VkPipelineStageFlagBits stage_dst) 
{
    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel   = 0;
    subresource_range.levelCount     = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount     = 1;

    VkImageMemoryBarrier barrier = {};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout           = layout_old;
    barrier.newLayout           = layout_new;
    barrier.srcQueueFamilyIndex = 0;
    barrier.dstQueueFamilyIndex = 0;
    barrier.image               = image;
    barrier.subresourceRange    = subresource_range;

    vkCmdPipelineBarrier(command_buffer, stage_src, stage_dst, 0, 0, 0, 0, 0, 1, &barrier);
}

void vk_loop(struct vk_context* vk, struct render_group* render_group)
{
	printf("t: %f\n", render_group->t);
	// Create UBO
	struct vk_ubo ubo = {};
	{
		mat4 model = GLM_MAT4_IDENTITY_INIT;
		memcpy(ubo.model, model, sizeof(mat4));
		glm_rotate(ubo.model, render_group->t * radians(90.0f), (vec3){0.0f, 0.0f, 1.0f});

		glm_lookat((vec3){2.0f, 2.0f, 2.0f}, (vec3){0.0f, 0.0f, 0.0f}, (vec3){0.0f, 0.0f, 1.0f}, ubo.view);

		glm_perspective(radians(45.0f), (float)vk->swap_extent.width / (float)vk->swap_extent.height, 0.1f, 10.0f, ubo.proj);
		ubo.proj[1][1] *= -1;
	}
	memcpy(vk->host_visible_mapped, &ubo, sizeof(ubo));

	uint32_t image_idx;
	// NOTE - we are setting the semaphore_image_available to be signaled when the
	// image is acquired?
	VkResult res = vkAcquireNextImageKHR(
		vk->device, 
		vk->swapchain, 
		UINT64_MAX, 
		vk->semaphore_image_available, 
		VK_NULL_HANDLE, 
		&image_idx);
	if(res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
	{
		vk_create_swapchain(vk, true);
		return;
	}

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(vk->command_buffer, &begin_info);
	{
		// Transferring image layout from UNDEFINED to COLOR_ATTACHMENT_OPTIMAL.
		// TODO - We'll want one for the depth image as well.
		insert_image_memory_barrier(
			vk->command_buffer, 
			vk->swap_images[image_idx], 
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);

		VkRenderingAttachmentInfo color_attachment = {};
		color_attachment.sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		color_attachment.loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp            = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachment.imageView          = vk->render_view;
		color_attachment.resolveMode        = VK_RESOLVE_MODE_AVERAGE_BIT;
		color_attachment.resolveImageView   = vk->swap_views[image_idx];
		color_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_GENERAL;
		color_attachment.clearValue.color   = 
			(VkClearColorValue)
			{{
				render_group->clear_color.r, 
				render_group->clear_color.g, 
				render_group->clear_color.b, 
				1.0f
			}};

		VkRenderingInfo render_info = {};
		render_info.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
		render_info.renderArea           = (VkRect2D){{0, 0}, vk->swap_extent};
		render_info.layerCount           = 1;
		render_info.colorAttachmentCount = 1;
		render_info.pColorAttachments    = &color_attachment;
		render_info.pDepthAttachment     = 0; // TODO - depth buffering
		render_info.pStencilAttachment   = 0; // TODO - depth buffering

		vkCmdBeginRendering(vk->command_buffer, &render_info);
		{
			// TODO - confused. does this actually need to be set up in init as well?
			// Try without, or something.
			VkViewport viewport = {};
			viewport.x        = 0.0f;
			viewport.y        = 0.0f;
			viewport.width    = (float)vk->swap_extent.width;
			viewport.height   = (float)vk->swap_extent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(vk->command_buffer, 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.offset = (VkOffset2D){0, 0};
			scissor.extent = vk->swap_extent;
			vkCmdSetScissor(vk->command_buffer, 0, 1, &scissor);

			vkCmdBindPipeline(
				vk->command_buffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				vk->pipeline);

			VkDeviceSize offs[] = {vk->vertex_buffer_offset};
			vkCmdBindVertexBuffers(
				vk->command_buffer, 
				0, 
				1, 
				&vk->device_local_buffer,
				offs);
			vkCmdBindIndexBuffer(
				vk->command_buffer, 
				vk->device_local_buffer, 
				vk->index_buffer_offset, 
				VK_INDEX_TYPE_UINT16);

			vkCmdBindDescriptorSets(
				vk->command_buffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				vk->pipeline_layout, 
				0, 
				1, 
				&vk->descriptor_set,
				0,
				0);
			
			vkCmdDrawIndexed(vk->command_buffer, INDICES_LEN, 1, 0, 0, 0);
		}
		vkCmdEndRendering(vk->command_buffer);

		// Transferring image layout from UNDEFINED to COLOR_ATTACHMENT_OPTIMAL.
		// TODO - We'll want one for the depth image as well.
		insert_image_memory_barrier(
			vk->command_buffer, 
			vk->swap_images[image_idx], 
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	}
	vkEndCommandBuffer(vk->command_buffer);

	VkPipelineStageFlags wait_stages[] = 
	{
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	// We wait to submit until that images is available from before. We did all
	// this prior stuff in the meantime, in theory.
	VkSubmitInfo submit_info = {};
	submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount   = 1;
	submit_info.pWaitSemaphores      = &vk->semaphore_image_available;
	submit_info.pWaitDstStageMask    = wait_stages;
	submit_info.commandBufferCount   = 1;
	submit_info.pCommandBuffers      = &vk->command_buffer;
	submit_info.pSignalSemaphores    = &vk->semaphore_render_finished;
	submit_info.signalSemaphoreCount = 1;
	vkQueueSubmit(vk->queue_graphics, 1, &submit_info, VK_NULL_HANDLE);

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &vk->semaphore_render_finished;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &vk->swapchain;
	present_info.pImageIndices = &image_idx;

	res = vkQueuePresentKHR(vk->queue_graphics, &present_info); // TODO - try queue_present?
	if(res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
	{
		vk_create_swapchain(vk, true);
		return;
	}

	vkDeviceWaitIdle(vk->device);
}
