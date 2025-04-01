struct xcb_context 
{
	bool                running;
	float               time_since_start;
	struct timespec     time_prev;
	
	xcb_connection_t*   connection;
	xcb_screen_t*       screen;
	xcb_window_t        window;
	uint32_t            window_w;
	uint32_t            window_h;

	xcb_key_symbols_t*  keysyms;
	bool 				mouse_just_warped;
	bool				mouse_moved_yet;

	struct game_memory* memory_pool;
	size_t 				memory_pool_bytes;

	struct input_state  input;
	struct render_group render_group;
	struct vk_context   vk;
};
