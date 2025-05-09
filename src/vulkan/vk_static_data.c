// TODO - We actually don't need this many vertices. Maybe not worth fixing
// before moving on, but figured it's worth mentioning.
#define VERTICES_LEN 24
struct vk_vertex vertices[VERTICES_LEN] = {
    {{{{-0.5f,  0.5f, -0.5f}}}, {{{ 1.0f, 0.0f, 0.0f}}}},
    {{{{ 0.5f,  0.5f, -0.5f}}}, {{{ 0.0f, 1.0f, 0.0f}}}},
    {{{{ 0.5f, -0.5f, -0.5f}}}, {{{ 0.0f, 0.0f, 1.0f}}}},
    {{{{-0.5f, -0.5f, -0.5f}}}, {{{ 0.0f, 0.0f, 0.0f}}}}, 

    {{{{-0.5f, -0.5f,  0.5f}}}, {{{ 1.0f, 0.0f, 0.0f}}}},
    {{{{ 0.5f, -0.5f,  0.5f}}}, {{{ 0.0f, 1.0f, 0.0f}}}},
    {{{{ 0.5f,  0.5f,  0.5f}}}, {{{ 0.0f, 0.0f, 1.0f}}}},
    {{{{-0.5f,  0.5f,  0.5f}}}, {{{ 0.0f, 0.0f, 0.0f}}}},

    {{{{-0.5f,  0.5f,  0.5f}}}, {{{ 1.0f, 0.0f, 0.0f}}}},
    {{{{-0.5f,  0.5f, -0.5f}}}, {{{ 0.0f, 1.0f, 0.0f}}}},
    {{{{-0.5f, -0.5f, -0.5f}}}, {{{ 0.0f, 0.0f, 1.0f}}}},
    {{{{-0.5f, -0.5f,  0.5f}}}, {{{ 0.0f, 0.0f, 0.0f}}}},

    {{{{ 0.5f, -0.5f,  0.5f}}}, {{{ 1.0f, 0.0f, 0.0f}}}},
    {{{{ 0.5f, -0.5f, -0.5f}}}, {{{ 0.0f, 1.0f, 0.0f}}}},
    {{{{ 0.5f,  0.5f, -0.5f}}}, {{{ 0.0f, 0.0f, 1.0f}}}},
    {{{{ 0.5f,  0.5f,  0.5f}}}, {{{ 0.0f, 0.0f, 0.0f}}}},

    {{{{-0.5f, -0.5f, -0.5f}}}, {{{ 1.0f, 0.0f, 0.0f}}}},
    {{{{ 0.5f, -0.5f, -0.5f}}}, {{{ 0.0f, 1.0f, 0.0f}}}},
    {{{{ 0.5f, -0.5f,  0.5f}}}, {{{ 0.0f, 0.0f, 1.0f}}}},
    {{{{-0.5f, -0.5f,  0.5f}}}, {{{ 0.0f, 0.0f, 0.0f}}}},

    {{{{-0.5f,  0.5f,  0.5f}}}, {{{ 1.0f, 0.0f, 0.0f}}}},
    {{{{ 0.5f,  0.5f,  0.5f}}}, {{{ 0.0f, 1.0f, 0.0f}}}},
    {{{{ 0.5f,  0.5f, -0.5f}}}, {{{ 0.0f, 0.0f, 1.0f}}}},
    {{{{-0.5f,  0.5f, -0.5f}}}, {{{ 0.0f, 0.0f, 0.0f}}}}
};

#define INDICES_LEN 36
uint16_t indices[INDICES_LEN] = 
{
	0, 1, 2, 
	2, 3, 0,

	4, 5, 6, 
	6, 7, 4,

	 8,  9, 10, 
	10, 11,  8,

	12, 13, 14, 
	14, 15, 12,

	16, 17, 18, 
	18, 19, 16,

	20, 21, 22, 
	22, 23, 20,
};
