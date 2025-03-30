void xcb_loop(struct xcb_context* xcb)
{
	while(xcb->running)
	{
		xcb_generic_event_t* e;
		while((e = xcb_poll_for_event(xcb->connection)))
		{
			switch(e->response_type & ~0x80)
			{
				case XCB_CONFIGURE_NOTIFY:
				{
					xcb_configure_notify_event_t* ev = (xcb_configure_notify_event_t*)e;
					xcb->window_w = ev->width;
					xcb->window_h = ev->height;
				}
				case XCB_KEY_PRESS:
				{
					xcb_key_press_event_t* k_e = (xcb_key_press_event_t*)e;
					xcb_keysym_t keysym = xcb_key_press_lookup_keysym(xcb->keysyms, k_e, 0);
					switch(keysym)
					{
						case XCB_ESCAPE:
						{
							xcb->running = false;
							break;
						}
						default:
						{
							break;
						}
					}
				}
				default:
				{
					break;
				}
			}
		}
		game_loop(&xcb->render_group);
		vk_loop(&xcb->vk, &xcb->render_group);
	}
}
