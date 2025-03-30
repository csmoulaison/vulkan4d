VkResult xcb_create_surface_callback(void* platform_data, struct vk_context* vk)
{
	struct xcb_context* xcb = (struct xcb_context*)platform_data;
	
	VkXcbSurfaceCreateInfoKHR info = {};
	info.sType      = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	info.pNext      = 0;
	info.flags      = 0;
	info.connection = xcb->connection;
	info.window     = xcb->window;

	return vkCreateXcbSurfaceKHR(vk->instance, &info, 0, &vk->surface);
}

struct xcb_context xcb_init()
{
	struct xcb_context xcb;
	
	xcb.connection = xcb_connect(0, 0);
	// TODO - Handle more than 1 screen?
	xcb.screen = xcb_setup_roots_iterator(xcb_get_setup(xcb.connection)).data;

	// Window event registration
	uint32_t mask = XCB_CW_EVENT_MASK;
	uint32_t values[1];
	values[0] = 
		XCB_EVENT_MASK_EXPOSURE | 
		XCB_EVENT_MASK_KEY_PRESS | 
		XCB_EVENT_MASK_STRUCTURE_NOTIFY;
	
	xcb.window = xcb_generate_id(xcb.connection);
	xcb_create_window(
		xcb.connection,
		XCB_COPY_FROM_PARENT,
		xcb.window,
		xcb.screen->root,
		0, 0,
		480, 480,
		1,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		xcb.screen->root_visual,
		mask, values);
	xcb_map_window(xcb.connection, xcb.window);

	xcb_flush(xcb.connection);

	// This needs to be initialized in order for keysym lookups to work
	xcb.keysyms = xcb_key_symbols_alloc(xcb.connection);

	// VOLATILE - window_exts_len must equal length of window_exts.
	uint32_t window_exts_len = 2;
	char* window_exts[2] = 
	{
		"VK_KHR_surface",
		VK_KHR_XCB_SURFACE_EXTENSION_NAME
	};

	// TODO - can we get the width and height post window manager resize to pass
	// to vk_init? Alternatively, don't worry about it and just handle fullscreen.
	xcb_get_geometry_reply_t* geometry = xcb_get_geometry_reply(
		xcb.connection, 
		xcb_get_geometry(xcb.connection, xcb.window), 
		0);
	xcb.window_w = geometry->width;
	xcb.window_h = geometry->height;

	// TODO - doesn't match by the time we are making swapchain, so have to
	// hardcode it here. Whyyyyy?
	// Might have to resort to just waiting until xcb_loop COnfigureNotify to do this.
	xcb.vk = vk_init(
		window_exts, 
		window_exts_len, 
		xcb_create_surface_callback, 
		(void*)&xcb);

	xcb.running = true;
	return xcb;
}
