struct xcb_context 
{
	bool running;
	
	xcb_connection_t* connection;
	xcb_screen_t* screen;
	xcb_window_t window;
	uint32_t window_w;
	uint32_t window_h;
	xcb_key_symbols_t* keysyms;

	struct render_group render_group;
	struct vk_context vk;
};
