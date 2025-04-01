#define BILLION 1000000000.0f

void xcb_loop(struct xcb_context* xcb)
{
	while(xcb->running)
	{
    	input_reset_buttons(&xcb->input);
    	
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
                		case XCB_W:
                		{
                    		input_button_press(&xcb->input.move_forward);
        					break;
                		}
                		case XCB_A:
                		{
                    		input_button_press(&xcb->input.move_left);
        					break;
                		}
                		case XCB_S:
                		{
                    		input_button_press(&xcb->input.move_back);
        					break;
                		}
                		case XCB_D:
                		{
                    		input_button_press(&xcb->input.move_right);
        					break;
                		}
                		default:
                    	{
                        	break;
                        }
            		}
            		break;
				}
				case XCB_KEY_RELEASE:
				{
					xcb_key_press_event_t* k_e = (xcb_key_press_event_t*)e;
					xcb_keysym_t keysym = xcb_key_press_lookup_keysym(xcb->keysyms, k_e, 0);
					switch(keysym)
					{
                		case XCB_W:
                		{
                    		input_button_release(&xcb->input.move_forward);
        					break;
                		}
                		case XCB_A:
                		{
                    		input_button_release(&xcb->input.move_left);
        					break;
                		}
                		case XCB_S:
                		{
                    		input_button_release(&xcb->input.move_back);
        					break;
                		}
                		case XCB_D:
                		{
                    		input_button_release(&xcb->input.move_right);
        					break;
                		}
                		default:
                    	{
                        	break;
                        }
            		}
            		break;
				}
				default:
				{
					break;
				}
			}
		}

        struct timespec time_cur;
        if(clock_gettime(CLOCK_REALTIME, &time_cur))
        {
    		PANIC();
        }
    	float dt = time_cur.tv_sec - xcb->time_prev.tv_sec + 
    	(float)time_cur.tv_nsec / BILLION - (float)xcb->time_prev.tv_nsec / BILLION;
        xcb->time_prev = time_cur;
    	xcb->time_since_start += dt;

		game_loop(
    		xcb->memory_pool,
    		xcb->memory_pool_bytes,
    		dt,
    		&xcb->input,
    		&xcb->render_group);

		xcb->render_group.t = xcb->time_since_start / 4.0f;
		vk_loop(&xcb->vk, &xcb->render_group);
	}
}
