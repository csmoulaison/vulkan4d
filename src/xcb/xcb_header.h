#define VK_USE_PLATFORM_XCB_KHR

// Taken from Xlib keysym defs which can be found at the following link:
// https://www.cl.cam.ac.uk/~mgk25/ucs/keysymdef.h
#define XCB_ESCAPE 0xff1b

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <vulkan/vulkan_xcb.h>
#include "xcb_structs.c"
#include "xcb_init.c"
#include "xcb_loop.c"
