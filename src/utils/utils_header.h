#define PANIC() printf("PANIC at %s:%u\n", __FILE__, __LINE__); exit(1)
// STL for now
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
// My utils, and cglm for math for now
#include "cglm/cglm.h"
#include "lerp.c"
#include "clamp.c"
#include "linalg.c"
