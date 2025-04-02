void game_init(void* mem, uint32_t mem_bytes)
{
    struct game_memory* game = (struct game_memory*)mem;

    game->t = 0;
    game->camera_position = v3_new(0.0f, 0.0f, 3.0f);
    game->camera_yaw = -90.0f;
    game->camera_pitch = 0.0f;
}
