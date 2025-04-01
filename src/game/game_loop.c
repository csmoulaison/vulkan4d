#define MAX_NETWORK_LINES 4
#define CAM_MOVE_SPEED 4.0f
#define CAM_LOOK_SPEED 6.0f

void game_loop(
    void* mem,
    size_t mem_bytes,
    float dt,
    struct input_state* input,
    struct render_group* render_group)
{
    struct game_memory* game = (struct game_memory*)mem;

	// TODO - wrap yaw
	game->camera_yaw   += (float)input->mouse_delta_x * CAM_LOOK_SPEED * dt;
	game->camera_pitch -= (float)input->mouse_delta_y * CAM_LOOK_SPEED * dt;
	if(game->camera_pitch > 90.0f) 
	{
		game->camera_pitch = 90.0f;
	}
	else if(game->camera_pitch < -90.0f)
	{
		game->camera_pitch = -90.0f;
	}

	struct v3 cam_forward = v3_new(
    	cos(radians(game->camera_yaw)) * cos(radians(game->camera_pitch)),
    	sin(radians(game->camera_pitch)),
    	sin(radians(game->camera_yaw)) * cos(radians(game->camera_pitch)));
	cam_forward = v3_normalize(cam_forward);

	struct v3 cam_up = v3_new(0.0f, 1.0f, 0.0f);
	struct v3 cam_right = v3_normalize(v3_cross(cam_forward, cam_up));

	if(input->move_forward.held) 
	{
		game->camera_position = v3_add(game->camera_position, v3_scale(cam_forward, CAM_MOVE_SPEED * dt));
	}
	if(input->move_back.held) 
    {
		game->camera_position = v3_sub(game->camera_position, v3_scale(cam_forward, CAM_MOVE_SPEED * dt));
	}
	if(input->move_left.held) 
	{
    	game->camera_position = v3_sub(
        	game->camera_position, 
        	v3_scale(cam_right, CAM_MOVE_SPEED * dt));
	}
	if(input->move_right.held) 
	{
    	game->camera_position = v3_add(
        	game->camera_position, 
        	v3_scale(cam_right, CAM_MOVE_SPEED * dt));
	}

	render_group->clear_color = v3_new(0.75, 0.75, 0.75);
	render_group->camera_position = game->camera_position;
	render_group->camera_target = v3_add(game->camera_position, cam_forward);
}
