#define VERTICES_LEN 4
struct vk_vertex vertices[VERTICES_LEN] =
{
	{{{{-0.5f, -0.5f}}}, {{{ 1.0f,  0.0f,  0.0f}}}},
    {{{{ 0.5f, -0.5f}}}, {{{ 0.0f,  1.0f,  0.0f}}}},
    {{{{ 0.5f,  0.5f}}}, {{{ 0.0f,  0.0f,  1.0f}}}},
    {{{{-0.5f,  0.5f}}}, {{{ 1.0f,  1.0f,  1.0f}}}}
};

#define INDICES_LEN 6
uint16_t indices[INDICES_LEN] = {0, 1, 2, 2, 3, 0};

struct vk_vertex_input_descriptions vk_vert_binding_descriptions =
{
	.binding = 
	{
		.binding = 0,
		.stride = sizeof(struct vk_vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	},
	.attributes = 
	{
		{
			.binding = 0,
			.location = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(struct vk_vertex, pos)
		},
		{
			.binding = 0,
			.location = 1,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(struct vk_vertex, color)
		}
	}
};
