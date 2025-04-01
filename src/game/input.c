// VOLATILE - this must match the number of buttons defined in input_state.
#define INPUT_BUTTONS_LEN 4

struct input_button 
{
	uint32_t held;
	uint32_t pressed;
	uint32_t released;
};
struct input_state
{
	int32_t mouse_delta_x;
	int32_t mouse_delta_y;
	int32_t mouse_x;
	int32_t mouse_y;

	union 
	{
    	struct input_button buttons[INPUT_BUTTONS_LEN];
    	struct 
    	{
        	struct input_button move_forward;
        	struct input_button move_back;
        	struct input_button move_left;
        	struct input_button move_right;
    	};
	};
};

void input_reset_buttons(struct input_state* input)
{
    for(uint32_t i = 0; i < INPUT_BUTTONS_LEN; i++) {
        input->buttons[i].pressed = 0;
        input->buttons[i].released = 0;
    }
}

void input_button_press(struct input_button* btn) 
{
    btn->pressed = 1;
    btn->held = 1;
}

void input_button_release(struct input_button* btn) 
{
    btn->held = 0;
    btn->released = 1;
}
