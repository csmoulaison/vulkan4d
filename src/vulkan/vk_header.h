#define VK_DEBUG 1
#define VK_IMMEDIATE 0

#define MAX_SWAP_IMAGES 4
#define MAX_IN_FLIGHT_FRAMES 2
#define DEPTH_ATTACHMENT_FORMAT VK_FORMAT_D32_SFLOAT

#include <vulkan/vulkan.h>
#include "vk_structs.c"
#include "vk_vertex.c"
#include "vk_init.c"
#include "vk_loop.c"

