#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "utils/utils_header.h"
#include "game/game_header.h"
#include "program_info.c"
#include "vulkan/vk_header.h"
#include "xcb/xcb_header.h"

int32_t main(int32_t argc, char** argv)
{
	struct xcb_context xcb = xcb_init();
	xcb_loop(&xcb);

	return 0;
}
